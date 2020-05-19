#ifndef FUNCTION_HPP
#define FUNCTION_HPP

#include "token.hpp"
#include <memory>
#include <vector>
#include <type_traits>

namespace mdf {
	
	//Alias
	typedef std::vector<std::shared_ptr<Token>> token_vector_t;
	
	/**
	 * @struct CallTuple
	 * @brief Struct di supporto per l'invocazione di una callable
	 */
 	template<size_t N> struct CallTuple
	{
		template<typename C, typename T, typename... P>
		inline static auto call_tuple(C & callable, T & tuple, token_vector_t& inputTokens, P&... p)
		{
			
			return CallTuple<N-1>::call_tuple(
				callable, tuple, inputTokens, 
				std::static_pointer_cast<TokenSlot<typename std::tuple_element<N-1, T>::type::Type>>(inputTokens.at(N - 1)) -> get_data(),
				p...);
				
		}
	};
	
	template<> struct CallTuple<0>
	{
		template<typename C, typename T, typename... P>
		inline static auto call_tuple(C & callable, T & tuple, token_vector_t& inputTokens, P&... p)
		{
			return callable(p...);
		}
	};   	
	
	
	/**
	 * @struct transfer_output_tokens
	 * @brief Struct di supporto per il trasferimento dei Tokens
	 */
	template<int index, typename... Ts>
	struct TransferOutputTokens {
		
		inline void operator() 
		(std::tuple<Ts...>& t, token_vector_t* output) 
		{ 
			output -> insert(output -> begin(), std::make_shared<TokenSlot<typename std::tuple_element<index, std::tuple<Ts...>>::type>>(std::get<index>(t)));
			TransferOutputTokens<index - 1, Ts...>{}(t, output);	 
		}
	};

	template<typename... Ts>
	struct TransferOutputTokens<0, Ts...> {
		inline void operator() 
		(std::tuple<Ts...>& t, token_vector_t* output) 
		{
			output -> insert(output -> begin(), std::make_shared<TokenSlot<typename std::tuple_element<0, std::tuple<Ts...>>::type>>(std::get<0>(t)));	 
		}
	};	
	
	/**
	 * @struct Param
	 * @tparam Il tipo incapsulato
	 * @brief Struct di supporto per incapsulare un tipo
	 */
	template <typename T>
	struct Param {
		using Type = T;
	};
	
	/**
	 * @class Function
	 * @brief La classe Function è una classe astratta per implementare
	 * le funzioni. Fornisce il metodo per invocare e avere le informazioni
	 * base della callable.
	 */
	class Function {
	public:
	
		/**
		 * @brief ritorna l'arietà della funzione
		 * 			(dimensione input)
		 */
		virtual size_t get_arity() const = 0;
		
		/**
		 * @brief Ritorna la dimensione dell'output della funzione
		 */
		virtual size_t get_output_size() const = 0;
		
		/**
		 * @brief Esegue la Function
		 * 
		 * @param Il vettore di token contenente gli argomenti di input
		 * @return Il vettore di token contenente l'output
		 */
		virtual token_vector_t* execute(token_vector_t& input) const = 0;
		
		/**
		* @brief Crea una nuova Function
		* 
		* @tparam C Il Tipo della Callable
		* @tparam ...Args Il tipo dei suoi parametri
		* 
		* @param callable La callable da invocare
		* @param params	La lista dei Param che incapsulano il tipo dei parametri
		*/
		template <typename C, typename ... Args>
		static std::shared_ptr<Function> function_create(C && callable, Param<Args> && ... params);  
		
	};
	
	/**
	 * @class FunctionImp
	 * @tparam C il tipo della callable
	 * @tparam Args... il tipo dei suoi argomenti
	 * @brief Implementazione della classe astratta Function.
	 */
	template <typename C, typename ... Args>
	class FunctionImp : public Function {
	public:
	
		FunctionImp(C && callable, Param<Args> && ... params);
	
		size_t get_arity() const;
		
		size_t get_output_size() const;
		
		token_vector_t* execute(token_vector_t& input) const;
	
	private:
	
		C	_callable;
		
		std::tuple<Param<Args>...> _args_tuple;
		
		size_t _arity;
		
		size_t _output_size;
		
	};
	
	class FunctionPlaceHolder : public Function {
	public:
		FunctionPlaceHolder() = default;
		size_t get_arity() const { return 0; }
		size_t get_output_size() const { return 0; }
		token_vector_t* execute(token_vector_t& input) const { return nullptr; }
	};

	template <typename C, typename ... Args>
	std::shared_ptr<Function> Function::function_create(C && callable, Param<Args> && ... params) {
		return std::make_shared<FunctionImp<C, Args...>>(std::forward<C>(callable), std::forward<Param<Args>>(params)...);
	}
	
	template <typename C, typename ... Args>
	inline FunctionImp<C, Args...>::FunctionImp(C && callable, Param<Args> && ... params) :
		_callable{std::forward<C>(callable)},
		_args_tuple{std::forward_as_tuple<Param<Args>...>(std::forward<Param<Args>>(params)...)},
		_arity{sizeof...(Args)},
		_output_size{std::tuple_size<typename std::result_of<C(Args...)>::type>::value}
	{}
	
	template <typename C, typename ... Args>
	size_t FunctionImp<C, Args...>::get_arity() const {
		return _arity;
	}
	
	template <typename C, typename ... Args>
	size_t FunctionImp<C, Args...>::get_output_size() const {
		return _output_size;
	}
	
	/**
	 * @brief Trasferisce gli elementi di una tupla in un vettore di Token
	 * 
	 * @tparam ...Ts il tipo degli elementi della tupla
	 * @param tokens La tupla contenente i dati
	 * @param output Il vettore dove inserire i Tokens
	 */
	template <typename ... Ts>
	inline void send_output(std::tuple<Ts...>& tokens, token_vector_t* output) {
		const auto size = std::tuple_size<std::tuple<Ts...>>::value;
		TransferOutputTokens<size - 1, Ts...>{}(tokens, output);			
	}
	
	template <typename C, typename ... Args>
	token_vector_t* FunctionImp<C, Args...>::execute(token_vector_t& input) const {
		token_vector_t* output_vector = new token_vector_t();
		output_vector -> reserve(_output_size);
		
		auto ret_tuple = call(_callable, _args_tuple, input);
		send_output(ret_tuple, output_vector);
		
		return output_vector;
	}
	  
    /**
     * @brief Invoca la callable su una lista di argomenti
     * 
     * @tparam C Il Tipo della Callable
     * @tparam ...Args Il tipo dei suoi parametri
     * 
     * @param callable La Callable da invocare
     * @param tuple La tupla contenente i Param per il casting
     * @param input_tokens Il vettore conenente i TokenSlot con gli argomenti
     */
     
    template<typename C, typename... Args>
	inline auto call(C & callable, std::tuple<Args...> tuple, token_vector_t& input_tokens)
	{
		return CallTuple<std::tuple_size<decltype(tuple)>::value>::call_tuple(callable, tuple, input_tokens);
	}
	
}

#endif /* FUNCTION_HPP */
