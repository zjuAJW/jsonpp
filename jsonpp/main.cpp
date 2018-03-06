#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "jsonpp.h"

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
using namespace jsonpp;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%s")
#define EXPECT_EQ_SIZE_T(expect,actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%zu")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define TEST_ERROR(error, json)\
	do{\
		LeptValue v(LEPT_FALSE);\
		EXPECT_EQ_INT(error,LeptJsonParser::lept_parse(json,&v));\
		EXPECT_EQ_INT(LEPT_NULL, v.type);\
	}while(0)

#define TEST_NUMBER(expect, json)\
	do{\
		auto v = Json::parse(json);\
		EXPECT_EQ_INT(Json::NUMBER,v.type());\
		EXPECT_EQ_DOUBLE(expect, v.number_value());\
		v.~v();\
	}while(0)

#define TEST_STRING(expect, json)\
	do{\
		auto v = Json::parse(json);\
		EXPECT_EQ_INT(Json::STRING,v.type());\
		EXPECT_EQ_STRING(expect, v.string_value());\
		v.~v();\
	}while(0)

void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	//TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

								  /* the smallest number > 1 */
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002");
	/* minimum denormal */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324");
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	/* Max subnormal double */
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	/* Min normal positive double */
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	/* Max double */
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
	TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}



void test_parse_array() {
	size_t i, j;
	auto v = Json::parse("[ ]");

	EXPECT_EQ_INT(Json::ARRAY, v.type());
	EXPECT_EQ_SIZE_T(0, v.size());

	v = Json::parse("[ null , false , true , 123 , \"abc\" ]");
	EXPECT_EQ_INT(Json::ARRAY, v.type());
	EXPECT_EQ_SIZE_T(5, v.size());
	EXPECT_EQ_INT(Json::NUL, v[0].type());
	EXPECT_EQ_INT(Json::BOOL, v[1].type());
	EXPECT_EQ_INT(Json::BOOL, v[2].type());
	EXPECT_EQ_INT(Json::NUMBER, v[3].type());
	EXPECT_EQ_INT(Json::STRING, v[4].type());
	EXPECT_EQ_DOUBLE(123.0, v[3].number_value());
	EXPECT_EQ_STRING("abc", v[4].string_value());

	v = Json::parse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]");
	EXPECT_EQ_INT(Json::ARRAY, v.type());
	EXPECT_EQ_INT(Json::ARRAY, v[3].type());
	EXPECT_EQ_SIZE_T(3, v[3].size());
	for (i = 0; i < 4; i++) {
		EXPECT_EQ_INT(Json::ARRAY, v[i].type());
		EXPECT_EQ_SIZE_T(i, v[i].size());
		for (j = 0; j < i; j++) {
			EXPECT_EQ_INT(Json::NUMBER, v[i][j].type());
			EXPECT_EQ_DOUBLE((double)j, v[i][j].number_value());
		}
	}
}

void test_parse_object() {
	auto v = Json::parse("{ }");
	size_t i;
	EXPECT_EQ_INT(Json::OBJECT, v.type());
	EXPECT_EQ_SIZE_T(0, v.size());

	v = Json::parse(
		" { \"n\" : null , \"f\" : false , \"t\" : true , \"i\" : 123 , \"s\" : \"abc\", \"a\" : [ 1, 2, 3 ],\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }} ");

	/*EXPECT_EQ_INT(LeptJsonParser::LEPT_PARSE_OK, LeptJsonParser::lept_parse(
		" { "
		"\"n\" : null , "
		"\"f\" : false , "
		"\"t\" : true , "
		"\"i\" : 123 , "
		"\"s\" : \"abc\", "
		"\"a\" : [ 1, 2, 3 ],"
		"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } "
		, &v2));*/
	EXPECT_EQ_INT(Json::OBJECT, v.type());
	EXPECT_EQ_SIZE_T(7, v.size());
	EXPECT_EQ_DOUBLE(123, v["i"].number_value());
	EXPECT_EQ_INT(Json::NUL, v["n"].type());
	EXPECT_EQ_INT(Json::BOOL, v["f"].type());
	EXPECT_EQ_INT(Json::BOOL, v["t"].type());
	EXPECT_EQ_INT(Json::NUMBER, v["i"].type());
	EXPECT_EQ_INT(Json::STRING, v["s"].type());
	EXPECT_EQ_INT(Json::ARRAY, v["a"].type());
	EXPECT_EQ_INT(Json::OBJECT, v["o"].type());
	EXPECT_EQ_STRING("abc", v["s"].string_value());
	EXPECT_EQ_SIZE_T(3, v["a"].size());
	for (i = 0; i < 3; i++) {
		EXPECT_EQ_INT(Json::NUMBER, v["a"][i].type());
		EXPECT_EQ_DOUBLE(i + 1.0, v["a"][i].number_value());
	}
	{
		for (i = 0; i < 3; i++) {
			EXPECT_EQ_INT(Json::NUMBER, v["o"][std::to_string(i + 1)].type());
			EXPECT_EQ_DOUBLE(i + 1.0, v["o"][std::to_string(i + 1)].number_value());
		}
	}
}

void test_parse() {
	test_parse_number();
	test_parse_string();
	test_parse_array();
	test_parse_object();
}
int main() {
	/*Json my_json = Json::object{
		{ "key1", "value1" },
	{ "key2", false },
	{ "key3", Json::array { 1, 2, 3 } },
	};
	std::string json_str = my_json.dump();
	std::cout << json_str << std::endl;*/

	
#ifdef _WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	getchar();
	return main_ret;
}