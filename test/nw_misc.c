#include <stdlib.h>
#include <netware.h>
#include <screen.h>

void _NonAppStop( void )
{
  if (getenv("_IN_NETWARE_BASH_") == NULL)
    pressanykey();
}

/*
static void test_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Test not implemented on this platform yet");
}
*/

