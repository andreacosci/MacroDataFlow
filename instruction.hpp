#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <string>
#include "graph.hpp"
#include <iostream>

namespace mdf {
	
	template <typename T>
	struct Param;

	/**
	@class Instruction
	@brief Classe per gestire le Istruzioni
	L'istruzione opera da wrapper per i Nodi del grafo Macro Data Flow.
	Permette di connettere istruzioni dello stesso grafo e clonare istruzioni
	tra diverse istanze di grafo.
	*/
	class Instruction {
		
		friend class Graph;
		friend class Mdf;
		
	public:
	
	
		Instruction() = default;
		
		Instruction(const Instruction & other);
		
		Instruction& operator = (const Instruction& other);
		
		/**
		 * @brief Ritorna l'id della funzione
		 */
		size_t	operator()();
	
		/**
		 * @brief Ritorna l'id della funzione
		 */	
		size_t	id();
		
		/**
		 * @brief Ritorna la dimensione dell'input della funzione
		 * 		  contenuta dall'istruzione
		 */
		size_t	input_size() const;
		
		/**
		 * @brief Ritorna la dimensione dell'output della funzione
		 * 			contenuta dall'istruzione
		 */
		size_t	output_size() const;
		
		/**
		 * @brief Controlla che le istruzioni provengano dallo stesso
		 * 			grafo
		 * 
		 * @param other le istruzioni da confrontare
		 * @return ture se appartengono tutte allo stesso grafo
		 */
		template <typename ... Ins>
		bool from_same_graph(const Ins & ... other) const;

		/**
		 * @brief Controlla che le istruzioni provengano dallo stesso
		 * 			grafo
		 * 
		 * @param other vettore di istruzioni
		 * @return ture se appartengono tutte allo stesso grafo
		 */		
		bool from_same_graph(const std::vector<Instruction>& other) const;
		
	private:
		
		Instruction(Node& node, uintptr_t graph_id);
		
		Instruction(Node* node, uintptr_t graph_id);
		
		Node* _node{nullptr};
		
		uintptr_t _graph_id{0};	
		
		int _last_token{0};
		
		int _last_output{0};
		
	};
	
	inline Instruction::Instruction(Node& node, uintptr_t graph_id) : 
		_node{&node}, 
		_graph_id{graph_id}
	{};
	
	inline Instruction::Instruction(Node* node, uintptr_t graph_id) : 
		_node{node},
		_graph_id{graph_id}
	{};
		
	inline Instruction::Instruction(const Instruction & other) :
		_node{other._node},
		_graph_id{other._graph_id}
	{}
		
	inline Instruction& Instruction::operator = (const Instruction& other) {
		_node = other._node;
		_graph_id = other._graph_id;
		
		return *this;
	}
	
	inline size_t Instruction::operator()() {
		return _node -> _node_id;
	}
	
	inline size_t Instruction::id() {
		return _node -> _node_id;
	}
	
	inline size_t Instruction::input_size() const {
		return _node -> input_size();
	}
	
	inline size_t Instruction::output_size() const {
		return _node -> output_size();
	}
	
	template <typename ... Ins>
	inline bool Instruction::from_same_graph(const Ins& ... other) const {
		return ((other._graph_id == _graph_id) && ...);
	}
	
	inline bool Instruction::from_same_graph(const std::vector<Instruction>& other) const {
		return std::all_of(other.begin(), other.end(), [this](const Instruction& ins) -> bool {
			return this -> from_same_graph(ins);
		});
	}
	
}

#endif /* INSTRUCTION_HPP */
