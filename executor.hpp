#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <vector>
#include <condition_variable>
#include <unordered_map>
#include <atomic>
#include "graph.hpp"
#include "mdf.hpp"

namespace mdf {
	
	/**
	 * @struct GraphHandler
	 * @brief  Struct rappresentante un grafo nella pool, mantiene i dati
	 * 			relativi ad una certa istanza durante l'esecuzione.
	 */
	struct GraphHandler {
		std::promise<token_vector_t*> _promise;
		Graph* _graph;
		uintptr_t	_id;
		bool	 	_deleted;
		
		GraphHandler(Graph& graph) :
			_graph{new Graph(graph)},
			_deleted{false}
		{_id = reinterpret_cast<uintptr_t>(_graph); }
		
		~GraphHandler() {
			if (!_deleted)
			delete _graph;
		}
	};

	/**
	 * @struct Job
	 * @brief Rappresenta un Job della threadpool.
	 * 		  Contiende l'handler del grafo e il nodo da eseguire
	 */
	struct Job {
		GraphHandler*	_handler;
		size_t			_node_id;
			
		Job() = default;
		
		Job(GraphHandler* handler, size_t node_id) :
			_handler{handler},
			_node_id{node_id}
		{}
		
		Job(const Job& other) :
			_handler{other._handler},
			_node_id{other._node_id}
		{}
	};

	class Executor {
	public:
		/**
		 * @brief Costruisce un grafo con n thread
		 * 
		 * @param thread_n il numero dei thread
		 */
		Executor(unsigned thread_n);
		~Executor();
		
		/**
		 * @brief Esegue un'istanza del grafo, passati gli argomenti di input
		 *
		 * @tparam Args... il tipo dei parametri
		 * @param graph il grafo da eseguire
		 * @param input_args gli argomenti di input del primo nodo
		 * 
		 * @return un future contenente il risultato dell'esecuzione
		 */
		template <typename ... Args>
		std::future<token_vector_t*> run(Mdf& graph, Args && ... input_args);
		
		template <typename ... Args>
		std::future<token_vector_t*> run(Mdf& graph, Args & ... input_args);
		
	private:
	
		std::vector<std::thread> 			 							_workers;
		std::queue<Job>			 			 							_job_queue;
		std::mutex				 			 							_mutex;
		std::condition_variable	 			 							_empty;
		std::vector<GraphHandler*>										_graph_pool;
		volatile bool					 	 							_stop;
	};
	
	inline Executor::Executor(unsigned thread_n = std::thread::hardware_concurrency()) 
	: _stop{false}
	{
		for(int i = 0; i < thread_n; i++) {
			_workers.emplace_back([this] {
				for(;;) {
						
					Job job;

					{
						std::unique_lock<std::mutex> lock(this->_mutex);
						this->_empty.wait(lock,
							[this]{ return this->_stop || !this->_job_queue.empty(); });
								
						if(this->_stop && this->_job_queue.empty())
							return;
								
						job = std::move(this->_job_queue.front());
						this->_job_queue.pop();
					}			
					
					Graph*	graph 	= job._handler -> _graph;
					Node* node 		= graph -> _nodes.at(job._node_id).get();
					
					auto output = node -> execute();		
						
					if (node -> _is_output) {
						
						job._handler -> _promise.set_value(output);
						job._handler -> _deleted = true;
						delete job._handler -> _graph;
						
					} else {
						
						graph -> transfer_tokens(output, *(node -> _output_map));
						
						for(const size_t & next : *(node -> _successors)) {
							
							if (graph -> _nodes.at(next) -> _tokens_count.load() == 0 &&
								!graph -> _nodes.at(next) -> _processed.test_and_set()) {
								{
									std::unique_lock<std::mutex> lock(this->_mutex);	
									this -> _job_queue.emplace(job._handler, next);
								}
								
								this -> _empty.notify_one();
							}
						} 
					
					}
			
				}			
					
			});
		}		
	}
	
	inline Executor::~Executor() {
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_stop = true;
		}
			
		_empty.notify_all();
		for(std::thread &worker: _workers)
			worker.join();		
			
		for(GraphHandler*& graphs : _graph_pool)
			delete graphs;
				
	}
	
	template <typename ... Args>
	inline std::future<token_vector_t*> Executor::run(Mdf& graph, Args && ... input_args) {
		
		graph.validate();
		
		GraphHandler* handler = new GraphHandler(*graph._graph);
		handler -> _graph -> send_input_tokens(std::forward<Args>(input_args)...);
		
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_job_queue.emplace(handler, handler -> _graph -> _input_node);
		}
			
		_empty.notify_one();
		_graph_pool.push_back(handler);
		
		return handler -> _promise.get_future();
	}
	
	
	template <typename ... Args>
	inline std::future<token_vector_t*> Executor::run(Mdf& graph, Args & ... input_args) {
		return run(graph, std::move(input_args)...);
	}
	
	
}


#endif /* EXECUTOR_HPP */
