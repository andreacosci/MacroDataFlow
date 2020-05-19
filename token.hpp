#ifndef TOKEN_HPP
#define TOKEN_HPP


	struct Token {
		
	};

	template <typename T>
	struct TokenSlot : public Token {
	public:
	
		TokenSlot() = default;
		
		TokenSlot(const TokenSlot& other) = delete;
		
		TokenSlot(T& data) : 
			_data{data} 
		{}
		
		T& get_data() {
			return _data;
		};
		
		static T& from_token(Token* slot) {
			return static_cast<TokenSlot<T>*>(slot) -> get_data();
		}
	
	private:
	
		T _data;
		
	};

#endif /* TOKEN_HPP */
