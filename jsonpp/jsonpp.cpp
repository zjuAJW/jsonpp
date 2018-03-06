
#include "jsonpp.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>

namespace jsonpp {


	using std::string;
	using std::vector;
	using std::map;
	using std::make_shared;
	using std::initializer_list;
	using std::move;
	using std::to_string;
	using std::nullptr_t;

	/* Helper for representing null - just a do-nothing struct, plus comparison
	* operators so the helpers in JsonValue work. We can't use nullptr_t because
	* it may not be orderable.
	*/
	struct NullStruct {
		bool operator==(NullStruct) const { return true; }
		bool operator<(NullStruct) const { return false; }
	};

	/* * * * * * * * * * * * * * * * * * * *
	* Serialization
	*/

	static string dump(NullStruct) {
		return "null";
	}

	static string dump(double value) {
		if (std::isfinite(value)) {
			return to_string(value);
		}
		else {
			return "null";
		}
	}

	static string dump(int value) {
		return to_string(value);
	}

	static string dump(bool value) {
		string ret = value ? "true" : "false";
		return ret;
	}

	static string dump(const string &value) {
		string ret = "\"";
		for (const auto &ch : value) {
			switch (ch) {
			case '\"': ret += "\\\""; break;
			case '\\': ret += "\\\\"; break;
			case '/':  ret += "/"; break;
			case '\b':  ret += "\\b"; break;
			case '\f':  ret += "\\f"; break;
			case '\n':  ret += "\\n"; break;
			case '\r':  ret += "\\r"; break;
			case '\t':  ret += "\\t"; break;
			default:
				if (ch < 0x20) {
					char buffer[7];
					snprintf(buffer, sizeof(buffer), "\\u%04X", ch);
					ret += buffer;
				}
				else
					ret += ch;
			}
		}
		ret += "\"";
		return ret;
	}

	static string dump(const Json::array &values) {
		bool first = true;
		string ret = "[";
		for (const auto &value : values) {
			if (!first)
				ret += ", ";
			ret += value.dump();
			first = false;
		}
		ret += "]";
		return ret;
	}

	static string dump(const Json::object &values) {
		bool first = true;
		string ret = "{";
		for (const auto &kv : values) {
			if (!first)
				ret += ", ";
			ret += dump(kv.first);
			ret += ": ";
			ret += kv.second.dump();
			first = false;
		}
		ret += "}";
		return ret;
	}

	string Json::dump() const {
		return v_ptr->dump();
	}

	/* * * * * * * * * * * * * * * * * * * *
	* Value wrappers
	*/

	template <Json::Type tag, typename T>
	class Value : public JsonValue {
	protected:

		// Constructors
		explicit Value(const T &value) : m_value(value) {}
		explicit Value(T &&value) : m_value(move(value)) {}

		// Get type tag
		Json::Type type() const override {
			return tag;
		}

		// Comparisons
		bool equals(const JsonValue * other) const override {
			return m_value == static_cast<const Value<tag, T> *>(other)->m_value;
		}
		bool less(const JsonValue * other) const override {
			return m_value < static_cast<const Value<tag, T> *>(other)->m_value;
		}

		const T m_value;
		string dump() const override { return jsonpp::dump(m_value); }
	};

	class JsonDouble final : public Value<Json::NUMBER, double> {
		double number_value() const override { return m_value; }
		int int_value() const override { return static_cast<int>(m_value); }
		bool equals(const JsonValue * other) const override { return m_value == other->number_value(); }
		bool less(const JsonValue * other)   const override { return m_value <  other->number_value(); }
	public:
		explicit JsonDouble(double value) : Value(value) {}
	};

	class JsonInt final : public Value<Json::NUMBER, int> {
		double number_value() const override { return m_value; }
		int int_value() const override { return m_value; }
		bool equals(const JsonValue * other) const override { return m_value == other->number_value(); }
		bool less(const JsonValue * other)   const override { return m_value <  other->number_value(); }
	public:
		explicit JsonInt(int value) : Value(value) {}
	};

	class JsonBoolean final : public Value<Json::BOOL, bool> {
		bool bool_value() const override { return m_value; }
	public:
		explicit JsonBoolean(bool value) : Value(value) {}
	};

	class JsonString final : public Value<Json::STRING, string> {
		const string &string_value() const override { return m_value; }
		size_t size() const;
	public:
		explicit JsonString(const string &value) : Value(value) {}
		explicit JsonString(string &&value) : Value(move(value)) {}
	};

	class JsonArray final : public Value<Json::ARRAY, Json::array> {
		const Json::array &array_items() const override { return m_value; }
		const Json & operator[](size_t i) const override;
		size_t size() const;
	public:
		explicit JsonArray(const Json::array &value) : Value(value) {}
		explicit JsonArray(Json::array &&value) : Value(move(value)) {}
	};

	class JsonObject final : public Value<Json::OBJECT, Json::object> {
		const Json::object &object_items() const override { return m_value; }
		const Json & operator[](const string &key) const override;
		size_t size() const;
	public:
		explicit JsonObject(const Json::object &value) : Value(value) {}
		explicit JsonObject(Json::object &&value) : Value(move(value)) {}
	};

	class JsonNull final : public Value<Json::NUL, NullStruct> {
	public:
		JsonNull() : Value({}) {}
	};

	/* * * * * * * * * * * * * * * * * * * *
	* Static globals - static-init-safe
	*/
	struct Statics {
		const std::shared_ptr<JsonValue> null = make_shared<JsonNull>();
		const std::shared_ptr<JsonValue> t = make_shared<JsonBoolean>(true);
		const std::shared_ptr<JsonValue> f = make_shared<JsonBoolean>(false);
		const string empty_string;
		const vector<Json> empty_vector;
		const map<string, Json> empty_map;
		Statics() {}
	};

	static const Statics & statics() {
		static const Statics s{};
		return s;
	}

	static const Json & static_null() {
		// This has to be separate, not in Statics, because Json() accesses statics().null.
		static const Json json_null;
		return json_null;
	}

	/* * * * * * * * * * * * * * * * * * * *
	* Constructors
	*/

	Json::Json() noexcept :                  	v_ptr(statics().null) {}
	Json::Json(nullptr_t) noexcept :			v_ptr(statics().null) {}
	Json::Json(double value) :					v_ptr(make_shared<JsonDouble>(value)) {}
	Json::Json(int value) :						v_ptr(make_shared<JsonInt>(value)) {}
	Json::Json(bool value) :					v_ptr(value ? statics().t : statics().f) {}
	Json::Json(const string &value) :			v_ptr(make_shared<JsonString>(value)) {}
	Json::Json(string &&value) :				v_ptr(make_shared<JsonString>(move(value))) {}
	Json::Json(const char * value) :			v_ptr(make_shared<JsonString>(value)) {}
	Json::Json(const Json::array &values) :		v_ptr(make_shared<JsonArray>(values)) {}
	Json::Json(Json::array &&values) :			v_ptr(make_shared<JsonArray>(move(values))) {}
	Json::Json(const Json::object &values) :	v_ptr(make_shared<JsonObject>(values)) {}
	Json::Json(Json::object &&values) :			v_ptr(make_shared<JsonObject>(move(values))) {}

	/* * * * * * * * * * * * * * * * * * * *
	* Accessors
	*/

	Json::Type					Json::type()                         const { return v_ptr->type(); }
	double						Json::number_value()                 const { return v_ptr->number_value(); }
	int							Json::int_value()                    const { return v_ptr->int_value(); }
	bool						Json::bool_value()                   const { return v_ptr->bool_value(); }
	const string &				Json::string_value()                 const { return v_ptr->string_value(); }
	const Json::array&			Json::array_items()					 const { return v_ptr->array_items(); }
	const Json::object&			Json::object_items()				 const { return v_ptr->object_items(); }
	const Json &				Json::operator[] (size_t i)          const { return (*v_ptr)[i]; }
	const Json &				Json::operator[] (const string &key) const { return (*v_ptr)[key]; }
	size_t						Json::size()						 const { return v_ptr->size(); }

	double                    JsonValue::number_value()              const { return 0; }
	int                       JsonValue::int_value()                 const { return 0; }
	bool                      JsonValue::bool_value()                const { return false; }
	const string &            JsonValue::string_value()              const { return statics().empty_string; }
	const Json::array &		  JsonValue::array_items()               const { return statics().empty_vector; }
	const Json::object &	  JsonValue::object_items()              const { return statics().empty_map; }
	const Json &              JsonValue::operator[] (size_t)         const { return static_null(); }
	const Json &              JsonValue::operator[] (const string &) const { return static_null(); }
	size_t					  JsonValue::size()						 const { return 0; }

	const Json & JsonObject::operator[] (const string &key) const {
		auto iter = m_value.find(key);
		return (iter == m_value.end()) ? static_null() : iter->second;
	}
	const Json & JsonArray::operator[] (size_t i) const {
		if (i >= m_value.size()) return static_null();
		else return m_value[i];
	}

	size_t JsonString::size() const {
		return m_value.size();
	}

	size_t JsonObject::size() const {
		return m_value.size();
	}

	size_t JsonArray::size() const {
		return m_value.size();
	}

	/* * * * * * * * * * * * * * * * * * * *
	* Comparison
	*/

	bool Json::operator== (const Json &other) const {
		if (v_ptr == other.v_ptr)
			return true;
		if (v_ptr->type() != other.v_ptr->type())
			return false;

		return v_ptr->equals(other.v_ptr.get());
	}

	bool Json::operator< (const Json &other) const {
		if (v_ptr == other.v_ptr)
			return false;
		if (v_ptr->type() != other.v_ptr->type())
			return v_ptr->type() < other.v_ptr->type();

		return v_ptr->less(other.v_ptr.get());
	}

	/* * * * * * * * * * * * * * * * * * * *
	* Parsing
	*/

	/* esc(c)
	*
	* Format char c suitable for printing in an error message.
	*/
	static inline string esc(char c) {
		char buf[12];
		if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
			snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
		}
		else {
			snprintf(buf, sizeof buf, "(%d)", c);
		}
		return string(buf);
	}

	static inline bool in_range(long x, long lower, long upper) {
		return (x >= lower && x <= upper);
	}

	namespace {
		/* JsonParser
		*
		* Object that tracks all state of an in-progress parse.
		*/
		struct JsonParser final {

			/* State
			*/
			const string &str;
			size_t i;
			bool failed;
			string err;

			JsonParser(const string & s, size_t _i, bool _failed = 0, string _err = "") :str(s), i(_i), failed(_failed), err(_err) {}

			/* fail(msg, err_ret = Json())
			*
			* Mark this parse as failed.
			*/
			Json fail(string &&msg) {
				return fail(move(msg), Json());
			}

			template <typename T>
			T fail(string &&msg, const T err_ret) {
				if (!failed)
					err = std::move(msg);
				failed = true;
				return err_ret;
			}

			bool isfinished() {
				return i >= str.size();
			}

			int remain_length() {
				return str.size() - i;
			}

			/* parse_whitespace()
			*
			* Advance until the current character is non-whitespace.
			*/
			void parse_whitespace() {
				while (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t')
					++i;
			}


			/* get_next_token()
			*
			* Return the next non-whitespace character. If the end of the input is reached,
			* flag an error and return 0.
			*/
			char get_next_token() {
				parse_whitespace();
				if (failed) return (char)0;
				if (i == str.size())
					return fail("unexpected end of input", (char)0);

				return str[i];
			}

			/* encode_utf8(pt, out)
			*
			* Encode pt as UTF-8 and add it to out.
			*/
			void encode_utf8(long pt, string & out) {
				if (pt < 0)
					return;

				if (pt < 0x80) {
					out += static_cast<char>(pt);
				}
				else if (pt < 0x800) {
					out += static_cast<char>((pt >> 6) | 0xC0);
					out += static_cast<char>((pt & 0x3F) | 0x80);
				}
				else if (pt < 0x10000) {
					out += static_cast<char>((pt >> 12) | 0xE0);
					out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
					out += static_cast<char>((pt & 0x3F) | 0x80);
				}
				else {
					out += static_cast<char>((pt >> 18) | 0xF0);
					out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
					out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
					out += static_cast<char>((pt & 0x3F) | 0x80);
				}
			}

			void encode_utf8(string &str, unsigned u) {
				if (u <= 0x7F)
					str.push_back(u);
				else if (u <= 0x7FF) {
					str.push_back(0xC0 | ((u >> 6) & 0xFF));
					str.push_back(0x80 | (u & 0x3F));
				}
				else if (u <= 0xFFFF) {
					str.push_back(0xE0 | ((u >> 12) & 0xFF));
					str.push_back(0x80 | ((u >> 6) & 0x3F));
					str.push_back(0x80 | (u & 0x3F));
				}
				else {
					assert(u <= 0x10FFFF);
					str.push_back(0xF0 | ((u >> 18) & 0xFF));
					str.push_back(0x80 | ((u >> 12) & 0x3F));
					str.push_back(0x80 | ((u >> 6) & 0x3F));
					str.push_back(0x80 | (u & 0x3F));
				}
			}

			/* encode_utf8(pt, out)
			*
			* Encode pt as UTF-8 and add it to out.
			*/
			bool parse_hex(unsigned *u) {
				char ch;
				*u = 0;
				for (int k = 0; k < 4; k++) {
					*u <<= 4;
					ch = str[i + k];
					if (ch >= '0' && ch <= '9')  *u |= ch - '0';
					else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
					else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
					else return 0;
				}
				return 1;
			}

			/* parse_string()
			*
			* Parse a string, starting at the current position.
			*/

			string parse_string() {
				EXPECT(str, i, '\"');
				string ret;
				unsigned u, u2;
				while (true) {
					if (isfinished()) {
						return fail("unexpected end of input in string", "");
					}
					char ch = str[i++];
					switch (ch) {
					case '\"':
						return ret;
					case '\\':
						if(isfinished()) return fail("unexpected end of input in string", "");
						switch (str[i++]) {
						case '\"': ret += "\""; break;
						case '\\': ret += "\\"; break;
						case '/':  ret += "/"; break;
						case 'b':  ret += "\b"; break;
						case 'f':  ret += "\f"; break;
						case 'n':  ret += "\n"; break;
						case 'r':  ret += "\r"; break;
						case 't':  ret += "\t"; break;
						case 'u':
							if (!parse_hex(&u))
								return fail("bad \\u escape : ", "");
							i += 4;
							if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
								if (str[i++] != '\\')
									return fail("INVALID_UNICODE_SURROGATE", "");
								if (str[i++] != 'u')
									return fail("INVALID_UNICODE_SURROGATE", "");
								if (!parse_hex(&u2))
									return fail("INVALID_UNICODE_SURROGATE", "");
								if (u2 < 0xDC00 || u2 > 0xDFFF)
									return fail("INVALID_UNICODE_SURROGATE", "");
								u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
								i += 4;
							}
							encode_utf8(ret, u);
							break;
						default:
							return fail("bad \\u escape : ", "");
						}
						break;
					default:
						if ((unsigned char)ch < 0x20) {
							return fail("LEPT_PARSE_INVALID_STRING_CHAR","");
						}
						ret += ch;
					}
				}
			}


			/* parse_number()
			*
			* Parse a double.
			*/

			Json parse_number() {
				auto start = i;
				if (str[i] == '-') ++i; 
				if (str[i] == '0') ++i;
				else {
					if (!is_digit19(str[i])) return fail("LEPT_PARSE_INVALID_VALUE");
					while (is_digit(str[i])) ++i;

				}
				if (str[i] == '.') {
					++i;
					if (!is_digit(str[i])) return fail("LEPT_PARSE_INVALID_VALUE");
					while (isdigit(str[i])) ++i;
				}
				if (i < str.size() && (str[i] == 'e' || str[i] == 'E')) {
					++i;
					if (i >= str.size()) return fail("LEPT_PARSE_INVALID_VALUE");
					if (str[i] == '-' || str[i] == '+') ++i;
					if (i >= str.size() || !is_digit(str[i])) return fail("LEPT_PARSE_INVALID_VALUE");
					while (i <= str.size() && is_digit(str[i])) ++i;
				}
				errno = 0;
				double result = strtod(str.substr(start, i - start).c_str(), NULL);
				if (errno == ERANGE)
					return fail("LEPT_PARSE_INVALID_VALUE", result);
				return result;

			}

			/* expect(str, res)
			*
			* Expect that 'str' starts at the character that was just read. If it does, advance
			* the input and return res. If not, flag an error.
			*/
			Json expect(const string &expected, Json res) {
				assert(i != 0);
				i--;
				if (str.compare(i, expected.length(), expected) == 0) {
					i += expected.length();
					return res;
				}
				else {
					return fail("parse error: expected " + expected + ", got " + str.substr(i, expected.length()));
				}
			}


			/* parse_json()
			*
			* Parse BOOL.
			*/
			Json parse_literal(const string& literal, Json ret) {
				EXPECT(str, i, literal[0]);
				if (remain_length() < (literal.size() - 1)) {
					return fail("LEPT_PARSE_INVALID_VALUE", Json());
				}
				else {
					for (int k = 0; k < literal.size() - 1; ++k) {
						if (str[i + k] != literal[k + 1]) {
							return fail("LEPT_PARSE_INVALID_VALUE");
						}
					}
				}
				i += (literal.size() - 1);
				return ret;
			}

			/* parse_json()
			*
			* Parse a JSON object.
			*/

			Json parse_object() {
				EXPECT(str, i, '{');
				Json::object ret;
				while (1) {
					string key("");
					char ch = get_next_token();
					if (failed) {
						return fail("LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET");
					}
					if (ch == '}') {
						++i;
						return ret;
					}
					if (ch != '\"')
						return fail("LEPT_PARSE_MISS_KEY");
					key = parse_string();
					if(failed)
						return Json();
					ch = get_next_token();
					if (isfinished() || ch != ':')
						return fail("LEPT_PARSE_INVALID_VALUE");
					++i;
					ch = get_next_token();
					ret[key] = parse_json();
					ch = get_next_token();
					if (!isfinished() && ch == ',') ++i;
				}
			}

			Json parse_array() {
				EXPECT(str, i, '[');
				char ch = get_next_token();
				Json::array ret;
				if (ch == ']') {
					++i;
					return ret;
				}
				while (1) {
					ch = get_next_token();
					if (failed) return Json();
					Json element = parse_json();
					if (failed) return Json();
					ret.push_back(element);
					ch = get_next_token();
					if (ch == ',') ++i;
					else if (ch == ']') {
						++i;
						return ret;
					}
					else
						return fail("LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET");
				}
			}

			Json parse_json() {
				char ch = get_next_token();
				switch (ch) {
				case 'n':
					return parse_literal("null", Json());
				case 't':
					return parse_literal("true", true);
				case 'f':
					return parse_literal("false", false);
				case '\"':
					return parse_string();
				case '[':
					return parse_array();
				case '{':
					return parse_object();
				default:
					return parse_number();
				}
			}


		};
	}//namespace {

	Json Json::parse(const string &in) {
		JsonParser parser{ in, 0, false};
		Json result = parser.parse_json();

		// Check for any trailing garbage
		parser.parse_whitespace();
		if (parser.failed)
			return Json();
		if (parser.i != in.size())
			return parser.fail("unexpected trailing " + esc(in[parser.i]));

		return result;
	}

} // namespace jsonpp