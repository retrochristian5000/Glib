/*
 * gi-generate-repository: GIR generation
 *
 * Copyright (c) 2026  Florian Leander Singer <sp1rit@disroot.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <gio/gio.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "serpent.h"
#include "scanner_resources.h"

extern PyMODINIT_FUNC PyInit__giscanner (void);

static gboolean
giscanner_fill_builtins (void)
{
	int rc;
	PyObject *builtins, *dict, *str;

	builtins = PyImport_ImportModule ("builtins");
	if (builtins == NULL)
		return FALSE;

	dict = PyModule_GetDict (builtins);
	if (dict == NULL)
		goto failure;

#define set_builtin(key, value)                                       \
	str = PyUnicode_FromString ((value));                         \
	if (str == NULL)                                              \
		{                                                     \
			g_critical ("Failed to set " key " builtin"); \
			goto failure;                                 \
		}                                                     \
	rc = PyDict_SetItemString (dict, (key), str);                 \
	Py_DECREF (str);                                              \
	if (rc < 0)                                                   \
		{                                                     \
			g_critical ("Failed to set " key " builtin"); \
			goto failure;                                 \
		}

	set_builtin ("GIR_DIR", GIR_DIR);

#undef set_builtin

	Py_DECREF (builtins);
	return TRUE;
failure:
	Py_DECREF (builtins);
	return FALSE;

}

static PyObject *
giscanner_get_args (int argc, char *argv[])
{
	PyObject *args = PyList_New (argc - 1);
	for (int i = 0; i < argc; i++)
		{
			if (i == argc-1)
				{
					if (argv[i][0] == '@') // last item starts with @
						{
							PyObject *argmod, *argfun;
							PyObject *arg, *rspargs, *res;

							argmod = PyImport_ImportModule ("scanner.rspfileargs");
							if (argmod == NULL)
								{
									g_critical ("Unable to load rsp argument parser");
									goto failure;
								}

							argfun = PyObject_GetAttrString (argmod, "get_rspfile_args");
							if (argfun == NULL || !PyCallable_Check (argfun))
								{
									g_critical ("Unable to find the rsp argument parser method");
									Py_DECREF (argmod);
									goto failure;
								}

							Py_DECREF (argmod);

							arg = PyUnicode_DecodeFSDefault (&argv[i][1]); // skip beginning @
							if (arg == NULL)
								{
									g_critical ("Failed decoding parameter \"%s\"", argv[i]);
									Py_DECREF (argfun);
									goto failure;
								}

							rspargs = PyObject_CallOneArg (argfun, arg);
							Py_DECREF (arg);
							Py_DECREF (argfun);
							if (rspargs == NULL)
								{
									g_critical ("Failed rsp param decoding failed");
									goto failure;
								}

							res = PyObject_CallMethod(args, "extend", "O", rspargs);
							g_assert (res == Py_None);

							Py_DECREF (res);
							Py_DECREF (rspargs);
						}
					else
						{
							PyObject *arg = PyUnicode_DecodeFSDefault (argv[i]);
							if (arg == NULL)
								{
									g_critical ("Failed decoding parameter \"%s\"", argv[i]);
									goto failure;
								}
							PyList_Append (args, arg);
						}
				}
			else
				{
					PyObject *arg = PyUnicode_DecodeFSDefault (argv[i]);
					if (arg == NULL)
						{
							g_critical ("Failed decoding parameter \"%s\"", argv[i]);
							goto failure;
						}
					PyList_SetItem (args, i, arg);
				}
		}

	return args;
failure:
	Py_DECREF (args);
	return NULL;
}

int
main (int argc, char *argv[])
{
	int ret;

	PyStatus status;
	PyConfig cfg;
	PyObject *scannermod, *scannermain;
	PyObject *args, *res;

	if (argc < 1)
		abort ();

	g_resources_register (scanner_get_resource ());
	PyImport_AppendInittab ("dumpprovider", PyInit_dumpprovider);
	PyImport_AppendInittab ("_giscanner", PyInit__giscanner);

	PyConfig_InitPythonConfig (&cfg);

	status = PyConfig_SetBytesString (&cfg, &cfg.program_name, argv[0]);
	if (PyStatus_Exception (status)) {
		PyConfig_Clear (&cfg);
		Py_ExitStatusException (status);
		abort ();
	}

	status = Py_InitializeFromConfig (&cfg);
	if (PyStatus_Exception (status))
		{
			g_critical ("Python initialization failed: %s", status.err_msg);
			PyConfig_Clear (&cfg);
			Py_ExitStatusException (status);
			abort ();
		}

	PyConfig_Clear (&cfg);

	serpent_initialize ();
	serpent_register_resource_path ("/org/gnome/glib/");

	if (!giscanner_fill_builtins ())
		{
			ret = 1;
			goto err_post_init;
		}

	scannermod = PyImport_ImportModule ("scanner.scannermain");
	if (scannermod == NULL)
		{
			g_critical ("Unable to load scanner module");
			ret = 1;
			goto err_post_init;
		}

	scannermain = PyObject_GetAttrString (scannermod, "scanner_main");
	if (scannermain == NULL || !PyCallable_Check (scannermain))
		{
			g_critical ("Failed to find the scanner main method");
			ret = 1;
			goto err_post_import;
		}

	args = giscanner_get_args (argc, argv);
	if (args == NULL)
		{
			ret = 1;
			goto err_post_main_lookup;
		}

	res = PyObject_CallOneArg (scannermain, args);
	if (res == NULL)
		{
			/* giscanner is likely to already have printed an error message.
			 * don't g_critical an additional - more useless one - here.
			 */
			ret = 1;
			goto err_post_arg_parse;
		}
	ret = PyLong_AsLong(res);
	Py_DECREF (res);

err_post_arg_parse:
	Py_DECREF (args);
err_post_main_lookup:
	Py_DECREF (scannermain);
err_post_import:
	Py_DECREF (scannermod);
err_post_init:
	if (PyErr_Occurred ())
		PyErr_Print ();

	if (Py_FinalizeEx() < 0 && ret == 0)
		ret = 120;

	return ret;
}
