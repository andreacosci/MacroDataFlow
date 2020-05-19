#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "function.hpp"
#include <string>
#include <atomic>
#include <stdexcept>
#include <bitset>
#include <algorithm>
#include <iostream>
#include <list> 
#include <limits.h> 
#include <math.h>

namespace mdf {
	
	//Forward Declaration
	class Instruction;
	class Mdf;
	
	//Alias
	typedef std::vector<std::pair<size_t, size_t>> token_map_t;
	
	typedef std::vector<size_t> node_vector_t;

	struct Bitmask {
	public:
		static const uint32_t MASK = 0xFFFFFFFF;
		uint32_t LAST_MASK = 0xFFFFFFFF;
		
		uint32_t _n;
		unsigned _array_size;
		std::vector<uint32_t> _array;
		
		Bitmask(uint32_t n)
		{
			_n = n;
			_array_size = (int) ceil((double) n / 32.0);
			_array = std::vector<uint32_t>(_array_size, 0x0);
			if (n % 32 != 0)
				LAST_MASK = ((1 << (n % 32)) - 1);
		}

		bool set(uint32_t x) {
			uint32_t base = x/32;
			uint32_t offset = x % 32;
			
			if (_array[base] & (1 << offset))
				return false;
			else 
				_array[base] |= (1 << offset);
			
			return true;
		}
		
		bool all_set() {
			for(int i = 0; i < (int) _array_size - 2; i++) {
				if (_array[i] != MASK) 
					return false;
			}
			
			return (_array[_array_size - 1] == LAST_MASK);
		}
		
		bool all_zeros() {
			for(int i = 0; i < (int) _array_size; i++) {
				if (_array[i] != 0) 
					return false;
			}
			
			return true;
		}

	};
		
	enum node_type {
		STANDARD,
		SPLIT,
		MERGE
	};
	
	class Node {
		
		friend class Instruction;
		
		friend class Graph;		
		
		friend class Mdf;		
		
		friend class Executor;
		
	public:
	
		Node(Node& node);
			
		Node(size_t node_id, Node& node);
			
		Node(size_t node_id, std::shared_ptr<Function>& function);
		
		Node(size_t node_id, node_type type, size_t size);
		
	private:
		
		token_vector_t* execute();
		
		size_t	successors_count() const;
			
		size_t	dependents_count() const;
			
		size_t	input_size() const;
			
		size_t	output_size() const;
			
		bool	is_output()	const;
		
		node_type _type;
			
		size_t _node_id;
			
		std::atomic_int	_tokens_count;
			
		token_vector_t _input_tokens;
			
		std::shared_ptr<node_vector_t> _successors;
			
		std::shared_ptr<Bitmask>	   _dependents;
			
		std::shared_ptr<token_map_t> _output_map;
			
		std::shared_ptr<Function> _function;
		
		size_t _input_size;
		
		size_t _output_size;
			
		bool	_is_output;
			
		bool 	_is_complete;
		
		std::atomic_flag _processed;
		
	};	
	
	class Graph {
		
		friend class Mdf;
		friend class Executor;
		
	public:
	
		Graph() :
			_input_node{-1},
			_output_node{-1},
			_counter{0}
		{};
		
		Graph(const Graph& other);
		
		Graph(Graph&& other);
		
	private:
	
		template <typename ... Args>
		void send_input_tokens(Args && ... args);

		std::vector<std::unique_ptr<Node>> _nodes;
		
		int _output_node;
		
		int _input_node;
		
		size_t _counter;
		
		template <typename C, typename ... Args>
		Node& emplace_back(C && callable, Param<Args> && ... params);
		
		Node& emplace_back(Node& instruction);
		
		Node& merge_node(size_t input);
		
		Node& split_node(size_t output);
		
		void mark_as_input(Node& node);
		
		void mark_as_output(Node& node);
		
		void transfer_tokens(token_vector_t* output, token_map_t& output_map);
		
		void check_node(size_t id, bool* visited, bool* stack);
		
		void check_graph();
		
	};
	
	inline Node::Node(Node& node) {
		_input_tokens	= token_vector_t(node._input_size);
		_node_id 		= node._node_id;
		_tokens_count 	= node._input_size;
		_successors		= node._successors;
		_dependents		= node._dependents;	
		_output_map		= node._output_map;	
		_output_size	= node._output_size;
		_function		= node._function;
		_is_output		= node._is_output;
		_type			= node._type;
	}
	
	inline Node::Node(size_t node_id, Node& node) {
		_is_complete 	= false;
		_is_output		= false;
		_input_size		= node._input_size;
		_node_id 		= node_id;
		_tokens_count 	= _input_size;
		_output_size	= node._output_size;
		_function		= node._function;		
		_output_map 	= std::make_shared<token_map_t>();
		_dependents		= std::make_shared<Bitmask>(_input_size);
		_successors		= std::make_shared<node_vector_t>();
		_type			= node._type;
	}
	
	inline Node::Node(size_t node_id, std::shared_ptr<Function>& function) {
		_is_complete 	= false;
		_is_output		= false;
		_node_id 		= node_id;
		_input_size		= function -> get_arity();
		_output_size	= function -> get_output_size();
		_tokens_count 	= _input_size;
		_function		= function;
		_output_map 	= std::make_shared<token_map_t>();
		_dependents		= std::make_shared<Bitmask>(_input_size);
		_successors		= std::make_shared<node_vector_t>();
		_type 			= STANDARD;
	}
	
	inline Node::Node(size_t node_id, node_type type, size_t size) {
		_is_complete 	= false;
		_is_output		= false;
		_node_id 		= node_id;
		_function		= std::make_shared<mdf::FunctionPlaceHolder>();
		_type 			= type;	
		
		if (type == MERGE) {
			_input_size		= size;
			_output_size	= 1;
		} else {
			_input_size		= 1;
			_output_size	= size;			
		}
		
		_tokens_count 	= _input_size;
		_output_map 	= std::make_shared<token_map_t>();
		_dependents		= std::make_shared<Bitmask>(_input_size);
		_successors		= std::make_shared<node_vector_t>();
	}
			
	inline token_vector_t*	Node::execute() {
		
		token_vector_t*  vec;
		
		switch(_type) {
			case STANDARD:
				return _function -> execute(_input_tokens);
				
			case MERGE:
				vec = new token_vector_t();
				vec -> push_back(std::make_shared<TokenSlot<token_vector_t>>(_input_tokens));
				
				return vec;
				
			case SPLIT:
				vec = new token_vector_t(_output_size);
				for(int i = 0; i < _output_size; i++) {
					vec -> at(i) = _input_tokens[0];
				}
				
				return vec;
			
			default:
				break;
		}
		
		return nullptr;
	}
		
	inline size_t	Node::successors_count() const {
		return _successors -> size();
	}
			
	inline size_t	Node::dependents_count() const {
		return _dependents -> _n;
	}
			
	inline size_t	Node::input_size() const {
		return _input_size;
	}
			
	inline size_t	Node::output_size() const {
		return _output_size;
	}
			
	inline bool		Node::is_output()	const {
		return _is_output;
	}
	
	inline Graph::Graph(const Graph& other) {
		_output_node = other._output_node;
		_input_node = other._input_node;	
		
		for(auto& node : other._nodes) {
			_nodes.push_back(std::make_unique<Node>(*node));
		}	
	}
		
	inline Graph::Graph(Graph&& other) {
		_nodes = std::move(other._nodes);
		_output_node = other._output_node;
		_input_node = other._input_node;
	}
	
	
	template<int index, typename... Ts>
	struct transfer_input_tokens {
		inline void operator() (std::tuple<Ts...>&& tuple, token_vector_t& vec) {
			vec[index] = std::make_shared<TokenSlot<typename std::tuple_element<index, std::tuple<Ts...>>::type>>(std::get<index>(tuple));
			transfer_input_tokens<index - 1, Ts...>{}(std::forward<std::tuple<Ts...>>(tuple), vec);
		}
	};

	template<typename... Ts>
	struct transfer_input_tokens<0, Ts...> {
		inline void operator() (std::tuple<Ts...>&& tuple, token_vector_t& vec) {
			vec[0] = std::make_shared<TokenSlot<typename std::tuple_element<0, std::tuple<Ts...>>::type>>(std::get<0>(tuple));
		}
	};
		 
	template <typename ... Args>
	inline void Graph::send_input_tokens(Args && ... args) {
		const auto size = sizeof...(Args);
		transfer_input_tokens<size - 1, Args...>{}(std::forward_as_tuple(args...), _nodes.at(_input_node) -> _input_tokens);
	}
	
	template <typename C, typename ... Args>
	inline Node& Graph::emplace_back(C && callable, Param<Args> && ... params) {
		size_t id = _nodes.size();
		std::shared_ptr<Function> foo = Function::function_create(std::forward<C>(callable), std::forward<Param<Args>>(params)...);
		
		_nodes.push_back(std::make_unique<Node>(id, foo));
		return (*_nodes.back());
	}
		
	inline Node& Graph::emplace_back(Node& node) {
		size_t id = _nodes.size();
		
		_nodes.push_back(std::make_unique<Node>(id, node));
		return (*_nodes.back());		
	}
	
	inline Node& Graph::merge_node(size_t input) {
		size_t id = _nodes.size();
		
		_nodes.push_back(std::make_unique<Node>(id, MERGE, input));
		return (*_nodes.back());		
	}
	
	inline Node& Graph::split_node(size_t output) {
		size_t id = _nodes.size();
		
		_nodes.push_back(std::make_unique<Node>(id, SPLIT, output));
		return (*_nodes.back());		
	}
	
	inline void Graph::transfer_tokens(token_vector_t* output, token_map_t& output_map) {
		int i = 0;
			
		for(const auto & token_info : output_map) {
			size_t node_id 	= std::get<0>(token_info);
			size_t token_id = std::get<1>(token_info);
				
			token_vector_t & vec = _nodes.at(node_id) -> _input_tokens;
			_nodes.at(node_id) -> _tokens_count.fetch_sub(1, std::memory_order_relaxed);
			
			vec[token_id] = output -> at(i++); 
		}
		
		delete output;
	}
	
	inline void Graph::check_node(size_t id, bool* visited, bool* stack) {
		if (!visited[id]) {
				
				_counter++;
			
				visited[id] = true;
				stack[id] = true;
				
				if (id != _output_node && _nodes.at(id) -> _output_map -> size() != _nodes.at(id) -> output_size()) {
					throw std::invalid_argument("Tutti i token devono essere collegati");
				}
				
				for(const size_t& adj : *(_nodes.at(id) -> _successors)) {
					if ( !visited[adj] )
						check_node(adj, visited, stack);
					else if (stack[adj])
						throw std::invalid_argument("è presente un ciclo");
				}
		}
		
		stack[id] = false;
	}
	
	inline void Graph::check_graph() {
		
		if (_input_node == _output_node || _input_node == -1 || _output_node == -1)
			throw std::invalid_argument("I nodi di input ed output devono essere distinti e settati");
			
		bool* visited = new bool[_nodes.size()];
		bool* stack   = new bool[_nodes.size()];
		
		for(int i = 0; i < _nodes.size(); i++) {
			visited[i] 	= false;
			stack[i] 	= false;
		}
		
		check_node(_input_node, visited, stack);
		
		delete visited;
		delete stack;
		
		if (_counter != _nodes.size()) 
			throw std::invalid_argument("Non sono raggiungibili tutti i nodi"); 
			
	}
		
}
	
#endif /* GRAPH_HPP */
	
