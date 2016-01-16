#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"

// helper macros
#define OP_TRUE(left, right, op, message)		\
  do {							\
    Value *result;					\
    MAKE_VALUE(result, DT_INT, -1);			\
    op(left, right, result);				\
    bool b = result->v.boolV;				\
    free(result);					\
    ASSERT_TRUE(b,message);				\
   } while (0)

#define OP_FALSE(left, right, op, message)		\
  do {							\
    Value *result;					\
    MAKE_VALUE(result, DT_INT, -1);			\
    op(left, right, result);				\
    bool b = result->v.boolV;				\
    free(result);					\
    ASSERT_TRUE(!b,message);				\
   } while (0)

// test methods
static void testValueSerialize (void);
static void testOperators (void);
static void testExpressions (void);

char *testName;

// main method
int 
main (void) 
{
  testName = "";

  testValueSerialize();
  testOperators();
  testExpressions();

  return 0;
}


// ************************************************************ 
void
testValueSerialize (void)
{
  testName = "test value serialization and deserialization";

  ASSERT_EQUALS_STRING(serializeValue(stringToValue("i10")), "10", "create Value 10");
  ASSERT_EQUALS_STRING(serializeValue(stringToValue("f5.3")), "5.300000", "create Value 5.3");
  ASSERT_EQUALS_STRING(serializeValue(stringToValue("sHello World")), "Hello World", "create Value Hello World");
  ASSERT_EQUALS_STRING(serializeValue(stringToValue("bt")), "true", "create Value true");
  ASSERT_EQUALS_STRING(serializeValue(stringToValue("btrue")), "true", "create Value true");

  TEST_DONE();
}

// ************************************************************ 
void
testOperators (void)
{
  Value *result;
  testName = "test value comparison and boolean operators";
  MAKE_VALUE(result, DT_INT, 0);
  
  // equality
  OP_TRUE(stringToValue("i10"),stringToValue("i10"), valueEquals, "10 = 10");
  OP_FALSE(stringToValue("i9"),stringToValue("i10"), valueEquals, "9 != 10");
  OP_TRUE(stringToValue("sHello World"),stringToValue("sHello World"), valueEquals, "Hello World = Hello World");
  OP_FALSE(stringToValue("sHello Worl"),stringToValue("sHello World"), valueEquals, "Hello Worl != Hello World");
  OP_FALSE(stringToValue("sHello Worl"),stringToValue("sHello Wor"), valueEquals, "Hello Worl != Hello Wor");

  // smaller
  OP_TRUE(stringToValue("i3"),stringToValue("i10"), valueSmaller, "3 < 10");
  OP_TRUE(stringToValue("f5.0"),stringToValue("f6.5"), valueSmaller, "5.0 < 6.5");

  // boolean
  OP_TRUE(stringToValue("bt"),stringToValue("bt"), boolAnd, "t AND t = t");
  OP_FALSE(stringToValue("bt"),stringToValue("bf"), boolAnd, "t AND f = f");

  OP_TRUE(stringToValue("bt"),stringToValue("bf"), boolOr, "t OR f = t");
  OP_FALSE(stringToValue("bf"),stringToValue("bf"), boolOr, "f OR f = f");

  TEST_CHECK(boolNot(stringToValue("bf"), result));
  ASSERT_TRUE(result->v.boolV, "!f = t");

  TEST_DONE();
}

// ************************************************************
void
testExpressions (void)
{
  Expr *op, *l, *r;
  Value *res;
  testName = "test complex expressions";
  
  MAKE_CONS(l, stringToValue("i10"));
  evalExpr(NULL, NULL, l, &res);
  OP_TRUE(stringToValue("i10"), res, valueEquals, "Const 10");
 
  MAKE_CONS(r, stringToValue("i20"));
  evalExpr(NULL, NULL, r, &res);
  OP_TRUE(stringToValue("i20"), res, valueEquals, "Const 20");

  MAKE_BINOP_EXPR(op, l, r, OP_COMP_SMALLER);
  evalExpr(NULL, NULL, op, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "Const 10 < Const 20");

  MAKE_CONS(l, stringToValue("bt"));
  evalExpr(NULL, NULL, l, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "Const true");

  r = op;
  MAKE_BINOP_EXPR(op, r, l, OP_BOOL_AND);
  evalExpr(NULL, NULL, op, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "(Const 10 < Const 20) AND true");

  TEST_DONE();
}
