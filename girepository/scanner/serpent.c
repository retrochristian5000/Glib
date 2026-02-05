/*
 * libserpent: GResources Python Loader
 *
 * Copyright (C) 2025/26  Florian Leander Singer <sp1rit@disroot.org>
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

#include "serpent.h"

#include <glib.h>
#include <gio/gio.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

typedef struct
{
	PyObject_HEAD
	GBytes *data;
	gchar *filename;
	PyObject *source_hash;
	PyObject *serialize;
	PyObject *deserialize;
	PyObject *fix_co_filename;
	PyObject *compile;
	PyObject *exec;
} GResourcesLoader;

static PyObject *
GResourcesLoader_new (G_GNUC_UNUSED PyTypeObject *subtype, G_GNUC_UNUSED PyObject *args, G_GNUC_UNUSED PyObject *kwds)
{
	PyErr_Format (PyExc_RuntimeError, "Cannot directly instance GResourcesLoader");
	return NULL;
}

static void
GResourcesLoader_dealloc (GResourcesLoader *self)
{
	Py_DECREF (self->source_hash);
	Py_DECREF (self->serialize);
	Py_DECREF (self->deserialize);
	Py_XDECREF (self->fix_co_filename);
	Py_DECREF (self->compile);
	Py_DECREF (self->exec);
	g_bytes_unref (self->data);
	g_free (self->filename);
	Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
GResourcesLoader_create_module (G_GNUC_UNUSED GResourcesLoader *self, G_GNUC_UNUSED PyObject *args)
{
	Py_RETURN_NONE;
}

static const guchar GResourcesLoader_cache_signature[8] = { 0xFF, 's', 'e', 'r', 0, 'p', 'y', 'c' };

static PyObject *
GResourcesLoader_get_code (GResourcesLoader *self)
{
	gsize size;
	const gchar *src;
	PyObject *src_bytes;
	PyObject *hash;
	gchar *cache_name;
	GFile *cache_file;
	GInputStream *istream;
	PyObject *compile_args;
	PyObject *compile_kwargs;
	PyObject *code;
	GFile *cache_dir;
	GOutputStream *ostream;
	GError *err = NULL;

	src = g_bytes_get_data (self->data, &size);
	src_bytes = PyBytes_FromStringAndSize (src, size);
	hash = PyObject_CallOneArg (self->source_hash, src_bytes);

	cache_name = g_strdelimit (g_strdup (self->filename), "/", '_');
	cache_file = g_file_new_build_filename (g_get_user_cache_dir (), "serpent", cache_name, NULL);
	g_free (cache_name);
	istream = G_INPUT_STREAM (g_file_read (cache_file, NULL, &err));
	if (istream)
		{
			gchar header[16];
			gsize n;
			gssize len;

			GOutputStream *buffer;
			PyObject *serialized;
			PyObject *deserialized;
			PyObject *ret;

			if (!g_input_stream_read_all (istream, header, sizeof header, &n, NULL, &err))
				{
					g_warning ("Failed to read python cache for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
					g_object_unref (istream);
					goto compile;
				}

			if (n != sizeof header || memcmp (&header[0], GResourcesLoader_cache_signature, 8) != 0)
				{
					g_warning ("Python cache for \"%s\" is corrupted", self->filename);
					g_object_unref (istream);
					goto compile;
				}

			if (PyBytes_Size (hash) != 8 || memcmp (&header[8], PyBytes_AsString (hash), 8) != 0)
				{
					g_info ("Python cache for \"%s\" is not up to date, it will be regenerated", self->filename);
					g_object_unref (istream);
					goto compile;
				}

			buffer = g_memory_output_stream_new_resizable ();
			len = g_output_stream_splice (buffer, istream, G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &err);
			g_object_unref (istream);
			if (len < 0)
				{
					g_warning ("Failed to read python cache for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
					g_object_unref (buffer);
					goto compile;
				}

			serialized = PyBytes_FromStringAndSize (
				g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (buffer)),
				g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (buffer))
			);
			g_object_unref (buffer);

			deserialized = PyObject_CallOneArg (self->deserialize, serialized);
			Py_DECREF (serialized);
			if (!deserialized)
				{
					g_warning ("Failed to deserialize the python cache for \"%s\"", self->filename);
					PyErr_Clear ();
					goto compile;
				}

			if (!PyCode_Check (deserialized))
				{
					g_warning ("Python cache for \"%s\" was not a code object", self->filename);
					Py_DECREF (deserialized);
					goto compile;
				}

			if (self->fix_co_filename)
				{
					ret = PyObject_CallFunction (self->fix_co_filename, "ON", deserialized, PyUnicode_FromString (self->filename));
					if (!ret)
						{
							g_warning ("Failed to fixup the cached python code for \"%s\"", self->filename);
							PyErr_Clear ();
							goto compile;
						}
					Py_DECREF (ret);
				}

			g_object_unref (cache_file);
			Py_DECREF (hash);
			Py_DECREF (src_bytes);
			return deserialized;
		}
	else
		{
			if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
				g_warning ("Failed to open python cache for \"%s\": %s", self->filename, err->message);
			g_clear_error (&err);
		}
compile:
	compile_args = Py_BuildValue("(ONs)",
		src_bytes,
		PyUnicode_FromString (self->filename),
		"exec");
	compile_kwargs = PyDict_New ();
	PyDict_SetItemString (compile_kwargs, "dont_inherit", Py_True);

	code = PyObject_Call (self->compile, compile_args, compile_kwargs);
	Py_DECREF (compile_args);
	Py_DECREF (compile_kwargs);

	if (!code) {
		g_critical ("Failed to compile \"%s\"", self->filename);
		Py_DECREF (hash);
		return NULL;
	}

	cache_dir = g_file_get_parent (cache_file);
	g_file_make_directory_with_parents (cache_dir, NULL, NULL);
	g_object_unref (cache_dir);
	ostream = G_OUTPUT_STREAM (g_file_replace (cache_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &err));
	if (ostream)
		{
			PyObject *serialized;

			if (!g_output_stream_write_all (ostream, GResourcesLoader_cache_signature, sizeof GResourcesLoader_cache_signature, NULL, NULL, &err))
				{
					g_warning ("Failed to write cache file for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
					g_object_unref (ostream);
					goto cache_write_fail;
				}

			if (PyBytes_Size (hash) != 8)
				{
					g_critical ("Failed to write cache file for \"%s\": Unexpected length of hash", self->filename);
					g_object_unref (ostream);
					goto cache_write_fail;
				}
			if (!g_output_stream_write_all (ostream, PyBytes_AsString (hash), 8, NULL, NULL, &err))
				{
					g_warning ("Failed to write cache file for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
					g_object_unref (ostream);
					goto cache_write_fail;
				}

			serialized = PyObject_CallOneArg (self->serialize, code);
			if (!g_output_stream_write_all (ostream, PyBytes_AsString (serialized), PyBytes_Size (serialized), NULL, NULL, &err))
				{
					g_warning ("Failed to write cache file for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
				}

			Py_DECREF (serialized);
			if (!g_output_stream_close (ostream, NULL, &err))
				{
					g_warning ("Failed to close the cache file stream for \"%s\": %s", self->filename, err->message);
					g_clear_error (&err);
				}
			g_object_unref (ostream);
		}
	else
		{
			g_warning ("Failed to open cache file for \"%s\" for writing: %s", self->filename, err->message);
			g_clear_error (&err);
		}
cache_write_fail:
	g_object_unref (cache_file);
	Py_DECREF (hash);
	return code;
}

static PyObject *
GResourcesLoader_exec_module (GResourcesLoader *self, PyObject *args)
{
	PyObject *module;
	PyObject *code;
	if (!PyArg_ParseTuple (args, "O:exec_module", &module))
		return NULL;

	code = GResourcesLoader_get_code (self);
	if (!code)
		return NULL;

	PyObject *res = PyObject_CallFunctionObjArgs (self->exec, code, PyModule_GetDict (module), NULL);
	if (res)
		{
			Py_DECREF (res);
			Py_RETURN_NONE;
		}
	else
		{
			return NULL;
		}
}

static PyMethodDef
GResourcesLoader_Methods[] =
{
	{"create_module", (PyCFunction)GResourcesLoader_create_module, METH_VARARGS, NULL},
	{"exec_module", (PyCFunction)GResourcesLoader_exec_module, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL}
};

static PyTypeObject
GResourcesLoader_Type =
{
	PyVarObject_HEAD_INIT (NULL, 0)
	.tp_name = "serpent.GResourcesLoader",
	.tp_basicsize = sizeof (GResourcesLoader),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = GResourcesLoader_new,
	.tp_dealloc = (destructor)GResourcesLoader_dealloc,
	.tp_methods = GResourcesLoader_Methods,
};

typedef struct
{
	PyObject_HEAD
	gchar *base_path;
	// functions
	PyObject *spec_from_loader;
	PyObject *source_hash;
	PyObject *serialize;
	PyObject *deserialize;
	PyObject *fix_co_filename;
	PyObject *compile;
	PyObject *exec;
} GResourcesMetaFinder;

static PyObject *
GResourcesMetaFinder_new(PyTypeObject *subtype, G_GNUC_UNUSED PyObject *args, G_GNUC_UNUSED PyObject *kwds)
{
	GResourcesMetaFinder *self = (GResourcesMetaFinder *)subtype->tp_alloc(subtype, 0);
	self->base_path = NULL;
	self->spec_from_loader = NULL;
	self->source_hash = NULL;
	self->serialize = NULL;
	self->deserialize = NULL;
	self->fix_co_filename = NULL;
	self->compile = NULL;
	self->exec = NULL;
	return (PyObject *)self;
}

static int
GResourcesMetaFinder_init(GResourcesMetaFinder *self, PyObject *args, G_GNUC_UNUSED PyObject *kwargs)
{
	const gchar *base;
	if (!PyArg_ParseTuple (args, "s:GResourcesMetaFinder", &base))
		{
			// for some reason we can't set an exception in here
			g_critical ("GResourcesMetaFinder initialized with invalid arguments");
			return -1;
		}

	self->base_path = g_strdup (base);

	PyObject *util = PyImport_ImportModule ("importlib.util");
	PyObject *dict = PyModule_GetDict (util);
	self->spec_from_loader = PyDict_GetItemString (dict, "spec_from_loader");
	self->source_hash = PyDict_GetItemString (dict, "source_hash");
	Py_INCREF (self->spec_from_loader);
	Py_INCREF (self->source_hash);
	Py_DECREF (util);

	PyObject *marshal = PyImport_ImportModule ("marshal");
	dict = PyModule_GetDict (marshal);
	self->serialize = PyDict_GetItemString (dict, "dumps");
	self->deserialize = PyDict_GetItemString (dict, "loads");
	Py_INCREF (self->serialize);
	Py_INCREF (self->deserialize);
	Py_DECREF (marshal);

	/*
	 * Attempts to get _imp._fix_co_filename. This function is used to
	 * adjust the usually immutable code object retrieved from the cache,
	 * in case the path it was compiled against (used in backtraces)
	 * does not match the path we want. As the cache key is the path with
	 * slashes (/) replaces with underscores (_), this only really happens
	 * if a file is moved unchanged to/from a subdirectory with the name of
	 * the subdirectory followed by an underscore being removed/added as a
	 * prefix.
	 * This function is rather private, with there being no gurantee that
	 * it will exist in the future, so we'll do some guarding arround that
	 * and live with potentially broken backtraces.
	 */
	PyObject *imp = PyImport_ImportModule ("_imp");
	if (imp)
		{
			dict = PyModule_GetDict (imp);
			self->fix_co_filename = PyDict_GetItemString (dict, "_fix_co_filename");
			if (!self->fix_co_filename)
				{
					g_warning ("Failed to get _fix_co_filename");
					PyErr_Clear ();
				}
			Py_XINCREF (self->fix_co_filename);
			Py_DECREF (imp);
		}
	else
		{
			g_warning ("Failed to import _imp");
			self->fix_co_filename = NULL;
			PyErr_Clear ();
		}

	PyObject *builtins = PyImport_ImportModule ("builtins");
	dict = PyModule_GetDict (builtins);
	self->compile = PyDict_GetItemString (dict, "compile");
	self->exec = PyDict_GetItemString (dict, "exec");
	Py_INCREF (self->compile);
	Py_INCREF (self->exec);
	Py_DECREF (builtins);

	return 0;
}

static void
GResourcesMetaFinder_dealloc(GResourcesMetaFinder *self)
{
	Py_DECREF (self->spec_from_loader);
	Py_DECREF (self->source_hash);
	Py_DECREF (self->serialize);
	Py_DECREF (self->deserialize);
	Py_XDECREF (self->fix_co_filename);
	Py_DECREF (self->compile);
	Py_DECREF (self->exec);
	g_free (self->base_path);
	Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
GResourcesMetaFinder_create_spec (GResourcesMetaFinder *self, PyObject *fullname, const gchar *filename, GBytes *data, gboolean is_package)
{

	GResourcesLoader *loader = PyObject_New (GResourcesLoader, &GResourcesLoader_Type);
	loader->filename = g_strdup (filename);
	loader->data = g_bytes_ref (data);
	loader->source_hash = Py_NewRef (self->source_hash);
	loader->serialize = Py_NewRef (self->serialize);
	loader->deserialize = Py_NewRef (self->deserialize);
	loader->fix_co_filename = Py_XNewRef (self->fix_co_filename);
	loader->compile = Py_NewRef (self->compile);
	loader->exec = Py_NewRef (self->exec);

	PyObject* args = Py_BuildValue("(OO)", fullname, (PyObject *)loader);
	PyObject* kwargs = PyDict_New ();
	PyDict_SetItemString (kwargs, "is_package", is_package ? Py_True : Py_False);

	PyObject *spec = PyObject_Call (self->spec_from_loader, args, kwargs);

	Py_DECREF (args);
	Py_DECREF (kwargs);
	Py_DECREF (loader);
	return spec;
}

static PyObject *
GResourcesMetaFinder_find_spec (GResourcesMetaFinder *self, PyObject *args, PyObject *kwargs)
{
	static gchar *kwlist[] = {"fullname", "path", "target", NULL};
	PyObject *fullname = NULL;
	PyObject *path = NULL;
	PyObject *target = NULL;

	if (!PyArg_ParseTupleAndKeywords (args, kwargs, "OO|O:find_spec", kwlist, &fullname, &path, &target))
		return NULL;

	GStrv searchpaths;
	if (path && PyList_Check (path))
		{
			GStrvBuilder *builder = g_strv_builder_new ();
			for (gssize i = 0; i < PyList_Size (path); i++)
				g_strv_builder_add (builder, PyUnicode_AsUTF8 (PyList_GetItem (path, i)));
			searchpaths = g_strv_builder_unref_to_strv (builder);
		}
	else
		{
			searchpaths = g_strdupv ((gchar*[]){ self->base_path, NULL });
		}

	// Get the segment after the last period
	const gchar *name = PyUnicode_AsUTF8 (fullname);
	for (const gchar *i = name; *i != '\0'; i++)
		if (i[0] == '.')
			name = &i[1];

	for (gsize i = 0; searchpaths[i] != NULL; i++)
		{
			gchar *respath = g_build_path ("/", searchpaths[i], name, "__init__.py", NULL);
			GBytes *data = g_resources_lookup_data (respath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
			if (data)
				{
					PyObject *spec = GResourcesMetaFinder_create_spec (self, fullname, respath, data, TRUE);
					g_free (respath);
					g_bytes_unref (data);

					PyObject *submodule_search = PyObject_GetAttrString (spec, "submodule_search_locations");
					respath = g_build_path ("/", searchpaths[i], name, NULL);
					PyObject *search = PyUnicode_FromString (respath);
					g_free (respath);
					PyList_Append (submodule_search, search);
					Py_DECREF (search);

					g_strfreev (searchpaths);
					return spec;
				}
			g_free (respath);

			gchar* filename = g_strconcat (name, ".py", NULL);
			respath = g_build_path ("/", searchpaths[i], filename, NULL);
			g_free (filename);
			data = g_resources_lookup_data (respath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
			if (data)
				{
					PyObject *spec = GResourcesMetaFinder_create_spec (self, fullname, respath, data, FALSE);
					g_free (respath);
					g_bytes_unref (data);

					g_strfreev (searchpaths);
					return spec;
				}
			g_free (respath);
		}


	g_strfreev (searchpaths);
	Py_RETURN_NONE;
}

static PyMethodDef
GResourcesMetaFinder_Methods[] =
{
	{"find_spec", (PyCFunction)G_CALLBACK(GResourcesMetaFinder_find_spec), METH_VARARGS | METH_KEYWORDS, NULL},
	{NULL, NULL, 0, NULL}
};

static PyTypeObject
GResourcesMetaFinder_Type =
{
	PyVarObject_HEAD_INIT (NULL, 0)
	.tp_name = "serpent.GResourcesMetaFinder",
	.tp_basicsize = sizeof (GResourcesMetaFinder),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = GResourcesMetaFinder_new,
	.tp_init = (initproc)GResourcesMetaFinder_init,
	.tp_dealloc = (destructor)GResourcesMetaFinder_dealloc,
	.tp_methods = GResourcesMetaFinder_Methods,
};

void serpent_initialize(void) {
	PyGILState_STATE gil;

	if (!Py_IsInitialized ())
		g_error("Py_Initialize () needs to be called before sepent_initialize ()");
	gil = PyGILState_Ensure ();
	PyType_Ready (&GResourcesMetaFinder_Type);
	PyType_Ready (&GResourcesLoader_Type);
	PyGILState_Release (gil);
}

void serpent_register_resource_path(const gchar* path) {
	PyGILState_STATE gil;
	PyObject *meta, *pypath, *finder;

	gil = PyGILState_Ensure ();
	meta = PySys_GetObject ("meta_path");

	pypath = PyUnicode_FromString (path);
	finder = PyObject_CallOneArg ((PyObject *)&GResourcesMetaFinder_Type, pypath);
	Py_DECREF (pypath);
	if (!finder) {
		g_critical ("Failed to creaate GResourcesMetaFinder, did you call serpent_initialize ()?");
		goto exit;
	}

	PyList_Insert (meta, 0, finder);
	Py_DECREF (finder);

exit:
	PyGILState_Release (gil);
}
