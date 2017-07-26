#include "TextureCube.hpp"

#include "Error.hpp"
#include "InvalidObject.hpp"
#include "Buffer.hpp"

#include "InlineMethods.hpp"

PyObject * MGLTextureCube_tp_new(PyTypeObject * type, PyObject * args, PyObject * kwargs) {
	MGLTextureCube * self = (MGLTextureCube *)type->tp_alloc(type, 0);

	#ifdef MGL_VERBOSE
	printf("MGLTextureCube_tp_new %p\n", self);
	#endif

	if (self) {
	}

	return (PyObject *)self;
}

void MGLTextureCube_tp_dealloc(MGLTextureCube * self) {

	#ifdef MGL_VERBOSE
	printf("MGLTextureCube_tp_dealloc %p\n", self);
	#endif

	MGLTextureCube_Type.tp_free((PyObject *)self);
}

int MGLTextureCube_tp_init(MGLTextureCube * self, PyObject * args, PyObject * kwargs) {
	MGLError_Set("cannot create mgl.TextureCube manually");
	return -1;
}

PyObject * MGLTextureCube_read(MGLTextureCube * self, PyObject * args) {
	int alignment;

	int args_ok = PyArg_ParseTuple(
		args,
		"I",
		&alignment
	);

	if (!args_ok) {
		return 0;
	}

	if (alignment != 1 && alignment != 2 && alignment != 4 && alignment != 8) {
		MGLError_Set("the alignment must be 1, 2, 4 or 8");
		return 0;
	}

	int expected_size = self->width * self->components * (self->floats ? 4 : 1);
	expected_size = (expected_size + alignment - 1) / alignment * alignment;
	expected_size = expected_size * self->height * 6;

	PyObject * result = PyBytes_FromStringAndSize(0, expected_size);
	char * data = PyBytes_AS_STRING(result);

	const int formats[] = {0, GL_RED, GL_RG, GL_RGB, GL_RGBA};

	int pixel_type = self->floats ? GL_FLOAT : GL_UNSIGNED_BYTE;
	int format = formats[self->components];

	const GLMethods & gl = self->context->gl;

	gl.ActiveTexture(GL_TEXTURE0 + self->context->default_texture_unit);
	gl.BindTexture(GL_TEXTURE_CUBE_MAP, self->texture_obj);

	gl.PixelStorei(GL_PACK_ALIGNMENT, alignment);
	gl.PixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, pixel_type, data + expected_size * 0 / 6);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, pixel_type, data + expected_size * 1 / 6);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, pixel_type, data + expected_size * 2 / 6);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, pixel_type, data + expected_size * 3 / 6);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, pixel_type, data + expected_size * 4 / 6);
	gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, pixel_type, data + expected_size * 5 / 6);

	return result;
}


PyObject * MGLTextureCube_read_into(MGLTextureCube * self, PyObject * args) {
	PyObject * data;
	int alignment;
	Py_ssize_t write_offset;

	int args_ok = PyArg_ParseTuple(
		args,
		"OIn",
		&data,
		&alignment,
		&write_offset
	);

	if (!args_ok) {
		return 0;
	}

	if (alignment != 1 && alignment != 2 && alignment != 4 && alignment != 8) {
		MGLError_Set("the alignment must be 1, 2, 4 or 8");
		return 0;
	}

	int expected_size = self->width * self->components * (self->floats ? 4 : 1);
	expected_size = (expected_size + alignment - 1) / alignment * alignment;
	expected_size = expected_size * self->height * 6;

	const int formats[] = {0, GL_RED, GL_RG, GL_RGB, GL_RGBA};

	int pixel_type = self->floats ? GL_FLOAT : GL_UNSIGNED_BYTE;
	int format = formats[self->components];

	if (Py_TYPE(data) == &MGLBuffer_Type) {

		MGLBuffer * buffer = (MGLBuffer *)data;

		const GLMethods & gl = self->context->gl;

		gl.BindBuffer(GL_PIXEL_PACK_BUFFER, buffer->buffer_obj);
		gl.ActiveTexture(GL_TEXTURE0 + self->context->default_texture_unit);
		gl.BindTexture(GL_TEXTURE_CUBE_MAP, self->texture_obj);
		gl.PixelStorei(GL_PACK_ALIGNMENT, alignment);
		gl.PixelStorei(GL_UNPACK_ALIGNMENT, alignment);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, pixel_type, (char *)write_offset + expected_size * 0 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, pixel_type, (char *)write_offset + expected_size * 1 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, pixel_type, (char *)write_offset + expected_size * 2 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, pixel_type, (char *)write_offset + expected_size * 3 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, pixel_type, (char *)write_offset + expected_size * 4 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, pixel_type, (char *)write_offset + expected_size * 5 / 6);
		gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	} else {

		Py_buffer buffer_view;

		int get_buffer = PyObject_GetBuffer(data, &buffer_view, PyBUF_WRITABLE);
		if (get_buffer < 0) {
			MGLError_Set("the buffer (%s) does not support buffer interface", Py_TYPE(data)->tp_name);
			return 0;
		}

		if (buffer_view.len < write_offset + expected_size) {
			MGLError_Set("the buffer is too small");
			PyBuffer_Release(&buffer_view);
			return 0;
		}

		char * ptr = (char *)buffer_view.buf + write_offset;

		const GLMethods & gl = self->context->gl;
		gl.ActiveTexture(GL_TEXTURE0 + self->context->default_texture_unit);
		gl.BindTexture(GL_TEXTURE_CUBE_MAP, self->texture_obj);
		gl.PixelStorei(GL_PACK_ALIGNMENT, alignment);
		gl.PixelStorei(GL_UNPACK_ALIGNMENT, alignment);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, pixel_type, ptr + expected_size * 0 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, pixel_type, ptr + expected_size * 1 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, pixel_type, ptr + expected_size * 2 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, pixel_type, ptr + expected_size * 3 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, pixel_type, ptr + expected_size * 4 / 6);
		gl.GetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, pixel_type, ptr + expected_size * 5 / 6);

		PyBuffer_Release(&buffer_view);

	}

	Py_RETURN_NONE;
}

PyObject * MGLTextureCube_use(MGLTextureCube * self, PyObject * args) {
	int index;

	int args_ok = PyArg_ParseTuple(
		args,
		"I",
		&index
	);

	if (!args_ok) {
		return 0;
	}

	const GLMethods & gl = self->context->gl;
	gl.ActiveTexture(GL_TEXTURE0 + index);
	gl.BindTexture(GL_TEXTURE_CUBE_MAP, self->texture_obj);

	Py_RETURN_NONE;
}

PyObject * MGLTextureCube_release(MGLTextureCube * self) {
	MGLTextureCube_Invalidate(self);
	Py_RETURN_NONE;
}

PyMethodDef MGLTextureCube_tp_methods[] = {
	{"use", (PyCFunction)MGLTextureCube_use, METH_VARARGS, 0},
	{"release", (PyCFunction)MGLTextureCube_release, METH_NOARGS, 0},
	{0},
};

MGLContext * MGLTextureCube_get_context(MGLTextureCube * self, void * closure) {
	Py_INCREF(self->context);
	return self->context;
}

PyObject * MGLTextureCube_get_glo(MGLTextureCube * self, void * closure) {
	return PyLong_FromLong(self->texture_obj);
}

PyGetSetDef MGLTextureCube_tp_getseters[] = {
	{(char *)"context", (getter)MGLTextureCube_get_context, 0, 0, 0},
	{(char *)"glo", (getter)MGLTextureCube_get_glo, 0, 0, 0},
	{0},
};

PyTypeObject MGLTextureCube_Type = {
	PyVarObject_HEAD_INIT(0, 0)
	"mgl.TextureCube",                                      // tp_name
	sizeof(MGLTextureCube),                                 // tp_basicsize
	0,                                                      // tp_itemsize
	(destructor)MGLTextureCube_tp_dealloc,                  // tp_dealloc
	0,                                                      // tp_print
	0,                                                      // tp_getattr
	0,                                                      // tp_setattr
	0,                                                      // tp_reserved
	0,                                                      // tp_repr
	0,                                                      // tp_as_number
	0,                                                      // tp_as_sequence
	0,                                                      // tp_as_mapping
	0,                                                      // tp_hash
	0,                                                      // tp_call
	0,                                                      // tp_str
	0,                                                      // tp_getattro
	0,                                                      // tp_setattro
	0,                                                      // tp_as_buffer
	Py_TPFLAGS_DEFAULT,                                     // tp_flags
	0,                                                      // tp_doc
	0,                                                      // tp_traverse
	0,                                                      // tp_clear
	0,                                                      // tp_richcompare
	0,                                                      // tp_weaklistoffset
	0,                                                      // tp_iter
	0,                                                      // tp_iternext
	MGLTextureCube_tp_methods,                              // tp_methods
	0,                                                      // tp_members
	MGLTextureCube_tp_getseters,                            // tp_getset
	0,                                                      // tp_base
	0,                                                      // tp_dict
	0,                                                      // tp_descr_get
	0,                                                      // tp_descr_set
	0,                                                      // tp_dictoffset
	(initproc)MGLTextureCube_tp_init,                       // tp_init
	0,                                                      // tp_alloc
	MGLTextureCube_tp_new,                                  // tp_new
};

MGLTextureCube * MGLTextureCube_New() {
	MGLTextureCube * self = (MGLTextureCube *)MGLTextureCube_tp_new(&MGLTextureCube_Type, 0, 0);
	return self;
}

void MGLTextureCube_Invalidate(MGLTextureCube * texture) {
	if (Py_TYPE(texture) == &MGLInvalidObject_Type) {

		#ifdef MGL_VERBOSE
		printf("MGLTextureCube_Invalidate %p already released\n", texture);
		#endif

		return;
	}

	#ifdef MGL_VERBOSE
	printf("MGLTextureCube_Invalidate %p\n", texture);
	#endif

	texture->context->gl.DeleteTextures(1, (GLuint *)&texture->texture_obj);

	Py_DECREF(texture->context);

	Py_TYPE(texture) = &MGLInvalidObject_Type;

	Py_DECREF(texture);
}
