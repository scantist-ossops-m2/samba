/*
   Unix SMB/CIFS implementation.

   Python interface to ldb.

   Copyright (C) 2007-2008 Jelmer Vernooij <jelmer@samba.org>

     ** NOTE! The following LGPL license applies to the ldb
     ** library. This does NOT imply that all of Samba is released
     ** under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _PYLDB_H_
#define _PYLDB_H_

#include <talloc.h>
#include "ldb_private.h"
#include "lib/replace/system/python.h"

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_context *ldb_ctx;
} PyLdbObject;

/* pyldb_Ldb_AS_LDBCONTEXT() does not check argument validity,
   pyldb_Ldb_AsLdbContext() does */
#define pyldb_Ldb_AS_LDBCONTEXT(pyobj) ((PyLdbObject *)pyobj)->ldb_ctx

#define pyldb_Ldb_AsLdbContext(pyobj)		\
	(pyldb_check_type(pyobj, "Ldb") ?	\
	 pyldb_Ldb_AS_LDBCONTEXT(pyobj) : NULL)

#define PyErr_LDB_OR_RAISE(py_ldb, ldb) \
	ldb = pyldb_Ldb_AsLdbContext(py_ldb); \
	if (!ldb) { \
		PyErr_SetString(PyExc_TypeError, "Ldb connection object required"); \
		return NULL; \
	}

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_dn *dn;
} PyLdbDnObject;

PyObject *pyldb_Dn_FromDn(struct ldb_dn *);
bool pyldb_Object_AsDn(TALLOC_CTX *mem_ctx, PyObject *object, struct ldb_context *ldb_ctx, struct ldb_dn **dn);
#define pyldb_Dn_AS_DN(pyobj) ((PyLdbDnObject *)pyobj)->dn

#define PyErr_LDB_DN_OR_RAISE(py_ldb_dn, dn) \
	if (!pyldb_check_type(py_ldb_dn, "Dn")) { \
		PyErr_SetString(PyExc_TypeError, "ldb Dn object required"); \
		return NULL; \
	} \
	dn = pyldb_Dn_AS_DN(py_ldb_dn);

bool pyldb_check_type(PyObject *obj, const char *type_name);

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_message *msg;
} PyLdbMessageObject;
#define pyldb_Message_AsMessage(pyobj) ((PyLdbMessageObject *)pyobj)->msg

/*
 * NOTE: el (and so the return value of
 * pyldb_MessageElement_AsMessageElement()) may not be a valid talloc
 * context, it could be part of an array
 */

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_message_element *el;
} PyLdbMessageElementObject;

#define pyldb_MessageElement_AsMessageElement(pyobj) ((PyLdbMessageElementObject *)pyobj)->el

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_parse_tree *tree;
} PyLdbTreeObject;
#define pyldb_Tree_AsTree(pyobj) ((PyLdbTreeObject *)pyobj)->tree

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	PyObject *msgs;
	PyObject *referals;
	PyObject *controls;
} PyLdbResultObject;

typedef struct {
	PyObject_HEAD
	TALLOC_CTX *mem_ctx;
	struct ldb_control *data;
} PyLdbControlObject;

void PyErr_SetLdbError(PyObject *error, int ret, struct ldb_context *ldb_ctx);

#define PyErr_LDB_ERROR_IS_ERR_RAISE(err,ret,ldb) do { \
	if (ret != LDB_SUCCESS) { \
		PyErr_SetLdbError(err, ret, ldb); \
		return NULL; \
	} \
} while(0)

#define PyErr_LDB_ERROR_IS_ERR_RAISE_FREE(err,ret,ldb,mem_ctx) do {	\
	if (ret != LDB_SUCCESS) { \
		PyErr_SetLdbError(err, ret, ldb); \
		TALLOC_FREE(mem_ctx);		  \
		return NULL; \
	} \
} while(0)

/* Picked out of thin air. To do this properly, we should probably have some part of the
 * errors in LDB be allocated to bindings ? */
#define LDB_ERR_PYTHON_EXCEPTION	142

#endif /* _PYLDB_H_ */
