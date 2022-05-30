//
// Author: Alexander Sholokhov <ra9yer@yahoo.com>
//
// License: MIT
//

#include "f_interop.h"

#if __GNUC__ > 7
#include <cstddef>
typedef size_t fortran_charlen_t;
#else
typedef int fortran_charlen_t;
#endif

extern "C"
{
    // --- Fortran routines ---

    void genmsk_128_90_(char msg0[], int* ichk, char msgsent[], int* i4tone, int* itype, fortran_charlen_t,
                        fortran_charlen_t);
}

void fortran_genmsk_128_90(char msg0[], int* ichk, char msgsent[], int* i4tone, int* itype)
{
    genmsk_128_90_(msg0, ichk, msgsent, i4tone, itype, 37, 37);
}
