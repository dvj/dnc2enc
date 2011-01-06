/******************************************************************************
 * $Id: s57.h 11537 2007-05-16 03:35:17Z warmerdam $
 *
 * Project:  S-57 Translator
 * Purpose:  Declarations for S-57 translator not including the
 *           binding onto OGRLayer/DataSource/Driver which are found in
 *           ogr_s57.h.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef _S57_H_INCLUDED
#define _S57_H_INCLUDED

#include "ogr_feature.h"

#define EMPTY_NUMBER_MARKER 2147483641  /* MAXINT-6 */

/* -------------------------------------------------------------------- */
/*      Various option strings.                                         */
/* -------------------------------------------------------------------- */
#define S57O_UPDATES "UPDATES"
#define S57O_LNAM_REFS "LNAM_REFS"
#define S57O_SPLIT_MULTIPOINT "SPLIT_MULTIPOINT"
#define S57O_ADD_SOUNDG_DEPTH "ADD_SOUNDG_DEPTH"
#define S57O_PRESERVE_EMPTY_NUMBERS "PRESERVE_EMPTY_NUMBERS"
#define S57O_RETURN_PRIMITIVES "RETURN_PRIMITIVES"
#define S57O_RETURN_LINKAGES "RETURN_LINKAGES"
#define S57O_RETURN_DSID     "RETURN_DSID"

#define S57M_UPDATES                    0x01
#define S57M_LNAM_REFS                  0x02
#define S57M_SPLIT_MULTIPOINT           0x04
#define S57M_ADD_SOUNDG_DEPTH           0x08
#define S57M_PRESERVE_EMPTY_NUMBERS     0x10
#define S57M_RETURN_PRIMITIVES          0x20
#define S57M_RETURN_LINKAGES            0x40
#define S57M_RETURN_DSID                0x80

/* -------------------------------------------------------------------- */
/*      RCNM values.                                                    */
/* -------------------------------------------------------------------- */

#define RCNM_FE         100     /* Feature record */

#define RCNM_VI         110     /* Isolated Node */
#define RCNM_VC         120     /* Connected Node */
#define RCNM_VE         130     /* Edge */
#define RCNM_VF         140     /* Face */

#define RCNM_DSID       10

#define OGRN_VI         "IsolatedNode"
#define OGRN_VC         "ConnectedNode"
#define OGRN_VE         "Edge"
#define OGRN_VF         "Face"

/* -------------------------------------------------------------------- */
/*      FRID PRIM values.                                               */
/* -------------------------------------------------------------------- */
#define PRIM_P          1       /* point feature */
#define PRIM_L          2       /* line feature */
#define PRIM_A          3       /* area feature */
#define PRIM_N          4       /* non-spatial feature  */

/************************************************************************/
/*                          S57ClassRegistrar                           */
/************************************************************************/

#define MAX_CLASSES 23000
#define MAX_ATTRIBUTES 25000
#define SAT_ENUM        'E'
#define SAT_LIST        'L'
#define SAT_FLOAT       'F'
#define SAT_INT         'I'
#define SAT_CODE_STRING 'A'
#define SAT_FREE_TEXT   'S'
#endif /* ndef _S57_H_INCLUDED */
