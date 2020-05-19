#ifndef MDF_HPP
#define MDF_HPP

#include "instruction.hpp"

namespace mdf {
	
	//forward declaration
	class Executor;
	
	/**
	 * @class Mdf
	 * @brief La classe che opera da wrapper per un grafo.
	 * Fornisce una serie di metodi per aggiungere istruzioni al grafo e 
	 * definire le dipendenze.
	 */
	class Mdf {
		
		friend class Executor;
		
	public:	
	
		Mdf();
		
		~Mdf(); 
		
		Mdf(const Mdf &) = delete;
	
		/**
		 * @brief Aggiunge un'nodo che incapsula la callable C di parametri
		 * 		Args.
		 * 
		 *@tparam C il tipo della callable
		 *@tparam Args... il tipo dei parametri
		 * 
		 *@param callable la callable
		 *@param params la lista dei parametri
		 * 
		 *@return l'istruzione che incapsula il nodo creato.
		 */
		template <typename C, typename ... Args>
		Instruction emplace_back(C && callable, Param<Args> && ... params);
		
		/**
		 * @brief Aggiunge un nodo di merge
		 * 
		 * @param input_size La dimensione dell'input del nodo
		 * @return La nuova Instruction creata
		 * 
		 */
		Instruction merge_node(size_t input_size);	
		
		/**
		 * @brief Aggiunge un nodo di split
		 * 
		 * @param input_size La dimensione dell'output del nodo
		 * @return La nuova Instruction creata
		 * 
		 */
		Instruction split_node(size_t output_size);	
		
		/**
		 * @brief Aggiunge una coppia dell'istruzione
		 * 
		 * @param instruction L'instruction da clonare
		 * @return La nuova Instruction creata
		 * 
		 * @note Le istruzioni possono provvenire da grafi differenti
		 */
		Instruction emplace_back(Instruction& instruction);	
		
		/**
		 * @brief permette di settare la mappa di output del nodo
		 * 
		 * @param instruction l'istruzione della quale definire l'output
		 * @param output_map la mappa di output
		 */
		void set_output(Instruction& instruction, token_map_t && output_map);
		
		/**
		 * @brief aggiunge un elemento alla mappa di output
		 * 
		 * @param instruction l'istruzione alla quale aggiungere l'elemento
		 * @param inst_coord  la coppia da aggiungere
		 */
		void add_output(Instruction& instruction, std::pair<size_t, size_t> && inst_coord);
		
		/**
		 * @brief Collega tutto l'ouptut dell'istruzione all'input dell'altra
		 * 
		 * @param instruction L'istruzione input
		 * @param other	l'istruzione alla quale indirizzare l'output
		 */
		void send_to(Instruction& instruction, Instruction& other);
		
		/**
		 * @brief Collega tutto l'output di un'istruzione a molteplici istruzioni
		 * 
		 * @param instruction L'istruzione dalla quale prelevare l'input
		 * @param head 		  la prima istruzione
		 * @param other... 	  le latre istruzioni
		 */
		template <typename ... Inst>
		void send_to(Instruction& instruction, Instruction& head, Inst& ... other);
		
		/**
		 * @brief Collega tutto l'output di un'istruzione a molteplici istruzioni
		 * 
		 * @param instruction L'istruzione dalla quale prelevare l'input
		 * @param other Il vettore contenente le istruzioni output
		 */
		void send_to(Instruction& instruction, std::vector<Instruction>& other);
		
		/**
		 * @brief Collega all'input dell'istruzione l'output di un'altra istruzione
		 * 
		 * @param instruction l'istruzione che deve ricevere i dati in input
		 * @param other	l'istruzione che invia i dati
		 */
		void gather_from(Instruction& instruction, Instruction& other);

		/**
		 * @brief Collega all'input dell'istruzione l'output di molteplici istruzioni
		 * 
		 * @param head la prima istruzione che invia i dati
		 * @param other	le altre istruzioni
		 */		
		template <typename ... Inst>
		void gather_from(Instruction& instruction, Instruction& head, Inst& ... other);
		
		/**
		 * @brief Collega all'input dell'istruzione l'output di molteplici istruzioni
		 * 
		 * @param other Il vettore delle istruzioni che inviano i dati
		 */		
		void gather_from(Instruction& instruction, std::vector<Instruction>& other);
		
		/**
		 * @brief Etichetta un'istruzione come il quella di input
		 * @param instruction L'istruzione da marcare
		 */
		void mark_as_input(Instruction& instruction);
		
		/**
		 * @brief Etichetta un'istruzione come il quella di output
		 * @param instruction L'istruzione da marcare
		 */
		void mark_as_output(Instruction& instruction);
		
		/**
		 * @brief Esegue il controllo di correttezza del grafo
		 */
		void validate();

	private:
	
		void add_successor(node_vector_t* node_vec, size_t nodeB);
		
		void add_dependent(Bitmask* node_vec, int32_t & nodeB);
		
		Graph* 		_graph;
		
		uintptr_t	_graph_id;
		
		bool _valid;
		
	};
	
	inline Mdf::Mdf() :
		_graph{new Graph()},
		_valid{false}
	{
		_graph_id = reinterpret_cast<uintptr_t>(_graph);
	}
	
	inline Mdf::~Mdf() {
		delete _graph;
	}
	
	template <typename C, typename ... Args>
	inline Instruction Mdf::emplace_back(C && callable, Param<Args> && ... params) {
		Node& node = (*_graph).emplace_back(std::forward<C>(callable), std::forward<Param<Args>>(params)...);
		
		return Instruction(node, _graph_id);
	}
	
	inline Instruction Mdf::emplace_back(Instruction& instruction) {
		if (instruction._node == nullptr)
			throw std::invalid_argument("L'istruzione non deve essere vuota");
			
		Node& node = (*_graph).emplace_back(*instruction._node);
		
		return Instruction(node,_graph_id);
	}
	
	inline Instruction Mdf::merge_node(size_t input_size) {
		if (input_size < 1)
			throw std::invalid_argument("La dimensione dell'input deve essere almeno 1");
			
		Node& node = (*_graph).merge_node(input_size);
		
		return Instruction(node, _graph_id);
	}
	
	inline Instruction Mdf::split_node(size_t output_size) {
		if (output_size < 1)
			throw std::invalid_argument("La dimensione dell'output deve essere almeno 1");
			
		Node& node = (*_graph).split_node(output_size);
		
		return Instruction(node, _graph_id);
	}
	
	inline void Mdf::add_successor(node_vector_t* node_vec, size_t nodeB) {
		if (std::find(node_vec -> begin(), node_vec -> end(), nodeB) == node_vec -> end()) {
			node_vec -> push_back(nodeB);
		}	
	}
	
	inline void Mdf::add_dependent(Bitmask* mask, int32_t & nodeB) {
		if (!mask -> set(nodeB))
			throw std::invalid_argument("Il token è già stato collegato");
	}
	
	inline void Mdf::validate() {
		if (!_valid) {
			_graph -> check_graph();
			_valid = true;
		}
	}
	
	inline void Mdf::set_output(Instruction& instruction, token_map_t&& output_map) {
		
		if (_valid)
			throw std::invalid_argument("Il grafo non può più essere modificato");
		
		if (_graph_id != instruction._graph_id)
			throw std::invalid_argument("Il nodo non appartiene a questo grafo");
		
		if (instruction._node -> _output_map -> size() > 0)
			throw std::invalid_argument("La mappa di output non è vuota");
	
		if (output_map.size() != instruction.output_size())
			throw std::invalid_argument("La mappa di output deve essere grande quanto l'output dell'istruzione");
		
		for(const auto& token_info : output_map) {
			
			size_t next_ins = std::get<0>(token_info);
			Node& next_node = *_graph -> _nodes.at(next_ins);
			int32_t token_id = std::get<1>(token_info);
			
			if (token_id >= next_node._input_size)
				throw std::invalid_argument( "Token Id fuori dal range" );
			
			if (next_ins == instruction._node -> _node_id) 
				throw std::invalid_argument( "Non puoi collegare un nodo a se stesso" );
			else {
				add_successor(instruction._node -> _successors.get(), next_ins);
				add_dependent(next_node._dependents.get(), token_id);
			}
		}
		
		instruction._node -> _output_map = std::make_shared<token_map_t>(std::forward<token_map_t>(output_map));
	}
	
	inline void Mdf::add_output(Instruction& instruction, std::pair<size_t, size_t> && inst_coord) {

		if (_valid)
			throw std::invalid_argument("Il grafo non può più essere modificato");
			
		if (_graph_id != instruction._graph_id)
			throw std::invalid_argument("Il nodo non appartiene a questo grafo");
			
		if (instruction._node -> _output_map -> size() >= instruction.output_size()) 
			throw std::invalid_argument("La mappa di output è piena");	
			
		size_t next_ins = std::get<0>(inst_coord);
		Node& next_node = *_graph -> _nodes.at(next_ins);
		int32_t token_id = std::get<1>(inst_coord);	
			
		if (token_id >= next_node._input_size)
			throw std::invalid_argument( "Token Id fuori dal range" );
			
		if (next_ins == instruction._node -> _node_id) 
			throw std::invalid_argument( "Non puoi collegare un nodo a se stesso" );		
		else {
			add_successor(instruction._node -> _successors.get(), next_ins);
			add_dependent(next_node._dependents.get(), token_id);
		}
		
		instruction._node -> _output_map -> push_back(inst_coord);
		
	}

	inline void Mdf::send_to(Instruction& instruction, Instruction& other) {
		
		if (!instruction.from_same_graph(other))
			throw std::invalid_argument("Le istruzioni devono essere dello stesso grafo");
			
		for(int i = 0; i < instruction.output_size(); i++) {
			add_output(instruction, {other(), instruction._last_output++});
		} 
	}
	
	template <typename ... Inst>
	void Mdf::send_to(Instruction& instruction, Instruction& head, Inst& ... other) {
		send_to(instruction, head);
		send_to(instruction, other...);
	}
	
	inline void Mdf::send_to(Instruction& instruction, std::vector<Instruction>& other) {
		
		if (!instruction.from_same_graph(other))
			throw std::invalid_argument("Le istruzioni devono essere dello stesso grafo");
		
		for(int i = 0; i < instruction.output_size();) {
			for(Instruction& ins : other) {
				for(int j = 0; j < ins.input_size(); j++) {
					add_output(instruction, {ins(), j});
					i++;
				}
			}
		}
	}
	
	inline void Mdf::gather_from(Instruction& instruction, Instruction& other) {
		if (!instruction.from_same_graph(other))
			throw std::invalid_argument("Le istruzioni devono essere dello stesso grafo");
			
		for(int j = 0; j < other.output_size(); j++) {
			add_output(other, {instruction(), instruction._last_token++});
		} 
	}
	
	template <typename ... Inst>
	void Mdf::gather_from(Instruction& instruction, Instruction& head, Inst& ... other) {
		gather_from(instruction, head);
		gather_from(instruction, other...);
	}
	
	inline void Mdf::gather_from(Instruction& instruction, std::vector<Instruction>& other) {
		
		if (!instruction.from_same_graph(other))
			throw std::invalid_argument("Le istruzioni devono essere dello stesso grafo");
			
		for(int i = 0; i < instruction.input_size();) {
			for(Instruction& ins : other) {
				for(int j = 0; j < ins.output_size(); j++) {
					add_output(ins, {instruction(), i++});
				}
			}
		}
	}
	
	inline void Mdf::mark_as_input(Instruction& instruction) {
		
		if (_valid)
			throw std::invalid_argument("Il grafo non può più essere modificato");
		
		if (instruction._node -> _output_map -> size() != instruction.output_size())
			throw std::invalid_argument("Il nodo non è stato collegato");
			
		if  (!instruction._node -> _dependents -> all_zeros())
			throw std::invalid_argument("Il nodo di input non può ricevere dati da altri nodi");
		
		if (_graph_id != instruction._graph_id)
			throw std::invalid_argument("Il nodo non appartiene a questo grafo");
			
		_graph -> _input_node = instruction();
	}
	
	inline void Mdf::mark_as_output(Instruction& instruction) {
		
		if (_valid)
			throw std::invalid_argument("Il grafo non può più essere modificato");
		
		if (instruction._node -> _output_map -> size() != 0)
			throw std::invalid_argument("Il nodo di output non può inviare dati ad altri nodi");
			
		if (!instruction._node -> _dependents -> all_set())
			throw std::invalid_argument("Il nodo di output non riceve tutti i token");
		
		if (_graph_id != instruction._graph_id)
			throw std::invalid_argument("Il nodo non appartiene a questo grafo");
			
		_graph -> _output_node = instruction();
		instruction._node -> _is_output = true;		
	}
	
}

#endif /* MDF_HPP */
