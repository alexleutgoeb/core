/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file PythonPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Supports Python-implemented plugins.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_PYTHON

#include <Python.h>
#include <structmember.h>

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/PythonPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/ExternalLearningHelper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

DLVHEX_NAMESPACE_BEGIN

#ifdef POSIX
	#define setenv(VAR, VAL, V) ::setenv(VAR, VAL, V)
	#define unsetenv(VAR) ::unsetenv(VAR)
#else
	void setenv(const char* var, const char* val, int v){
		SetEnvironmentVariable(var, val);
	}

	void unsetenv(const char* var){
		SetEnvironmentVariable(var, NULL);
	}
#endif

PythonPlugin::CtxData::CtxData()
{
}

PythonPlugin::PythonPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-pythonplugin[internal]", 2, 0, 0);
}

PythonPlugin::~PythonPlugin()
{
}

// output help message for this plugin
void PythonPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --pythonmodule=[MODULE]    Add Python module \"MODULE\" (file \"MODULE.py\") as new plugin." << std::endl;
}

// accepted options: --pythonmodule=[PATH]
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void PythonPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--pythonmodule=") )
		{
			ctxdata.pythonScripts.push_back(str.substr(std::string("--pythonmodule=").length()));
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"PythonPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

namespace
{

typedef PythonPlugin::CtxData CtxData;

class PythonRewriter:
	public PluginRewriter
{
private:
	PythonPlugin::CtxData& ctxdata;
public:
	PythonRewriter(PythonPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
	virtual ~PythonRewriter() {}

	virtual void rewrite(ProgramCtx& ctx);
};

void PythonRewriter::rewrite(ProgramCtx& ctx)
{
}

} // anonymous namespace

// rewrite program
PluginRewriterPtr PythonPlugin::createRewriter(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	return PluginRewriterPtr(new PythonRewriter(ctxdata));
}

// register auxiliary printer for strong negation auxiliaries
void PythonPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	RegistryPtr reg = ctx.registry();
}

namespace{

inline long IDToLong(ID id){
	long l = ((long)id.kind << 32 ) | ((long)id.address);
	DBGLOG(DBG, "Stored ID " << id << " as " << l);
	return l;
}

inline ID longToID(long l){
	ID id;
	id.kind = (l >> 32) & 0xFFFFFFFF;
	id.address = l & 0xFFFFFFFF;
	return id;
}

}

namespace PythonAPI{

ProgramCtx* emb_ctx;
const PluginAtom::Query* emb_query;
PluginAtom::Answer* emb_answer;
NogoodContainerPtr emb_nogoods;

#if 0
typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
	PyObject *kind;
	PyObject *address;
} ID;

static void ID_dealloc(ID* self){

	Py_XDECREF(self->kind);
	Py_XDECREF(self->address);
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject *ID_new(PyTypeObject *type, PyObject *args, PyObject *kwds){

	ID *self;
	self = (ID *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->kind = PyInt_FromLong(0);
		if (self->kind == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->address = PyInt_FromLong(0);
		if (self->address == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}

	return (PyObject *)self;
}

static int ID_init(ID *self, PyObject *args, PyObject *kwds){

	PyObject *kind = NULL, *address = NULL, *tmp;
	static char *kwlist[] = {"kind", "address", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, &kind, &address)) return -1; 
	if (kind) {
		tmp = self->kind;
		Py_INCREF(kind);
		self->kind = kind;
		Py_XDECREF(tmp);
	}
	if (address) {
		tmp = self->address;
		Py_INCREF(address);
		self->address = address;
		Py_XDECREF(tmp);
	}

	return 0;
}


static PyMemberDef ID_members[] = {
	{"kind", T_INT, offsetof(ID, kind), 0, "kind of ID"},
	{"address", T_INT, offsetof(ID, address), 0, "address of ID"},
	{NULL}  /* Sentinel */
};

static PyObject *ID_getValue(ID* self){

	static PyObject *format = NULL;
	PyObject *args, *result;

	if (format == NULL) {
		format = PyString_FromString("%s %s");
		if (format == NULL) return NULL;
	}

	if (self->kind == NULL) {
		PyErr_SetString(PyExc_AttributeError, "kind");
		return NULL;
	}

	if (self->address == NULL) {
		PyErr_SetString(PyExc_AttributeError, "address");
		return NULL;
	}

	args = Py_BuildValue("OO", self->kind, self->address);
	if (args == NULL) return NULL;

	result = PyString_Format(format, args);
	Py_DECREF(args);

	return result;
}

static PyMethodDef ID_methods[] = {
	{"getValue", (PyCFunction)ID_getValue, METH_NOARGS, "Return the value of the ID"},
	{NULL}
};

static PyTypeObject IDType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"ID.ID",                   /*tp_name*/
	sizeof(ID),                /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)ID_dealloc,    /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"ID objects",              /* tp_doc */
	0,		           /* tp_traverse */
	0,		           /* tp_clear */
	0,		           /* tp_richcompare */
	0,		           /* tp_weaklistoffset */
	0,		           /* tp_iter */
	0,		           /* tp_iternext */
	ID_methods,                /* tp_methods */
	ID_members,                /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)ID_init,         /* tp_init */
	0,                         /* tp_alloc */
	ID_new,                    /* tp_new */
	};
#endif

static PyObject* emb_getTuple(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.getAttributes: Expect exactly one parameter");
	dlvhex::ID atID = longToID(PyInt_AsLong(PyTuple_GetItem(args, 0)));
	if (!atID.isAtom() && !atID.isLiteral()) throw PluginError("dlvhex.getAttributes: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(atID);

	PyObject *pyAt = PyTuple_New(ogatom.tuple.size());
	for (int i = 0; i < ogatom.tuple.size(); ++i){
		PyTuple_SetItem(pyAt, i, PyInt_FromLong(IDToLong(ogatom.tuple[i])));
	}

	return pyAt;
}

static PyObject* emb_getTupleValues(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.getAttributeValues: Expect exactly one parameter");
	dlvhex::ID atID = longToID(PyInt_AsLong(PyTuple_GetItem(args, 0)));
	if (!atID.isAtom() && !atID.isLiteral()) throw PluginError("dlvhex.getAttributeValues: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(atID);

	PyObject *pyAt = PyTuple_New(ogatom.tuple.size());
	for (int i = 0; i < ogatom.tuple.size(); ++i){
		std::stringstream ss;
		RawPrinter printer(ss, emb_ctx->registry());
		printer.print(ogatom.tuple[i]);
		PyTuple_SetItem(pyAt, i, PyString_FromString(ss.str().c_str()));
	}

	return pyAt;
}

static PyObject* emb_getValue(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.getValue: Expect exactly one parameter");
	dlvhex::ID id = longToID(PyInt_AsLong(PyTuple_GetItem(args, 0)));
	if (id.isTerm() && id.isIntegerTerm()){
		return Py_BuildValue("i", id.address);
	}else{
		std::stringstream ss;
		RawPrinter printer(ss, emb_ctx->registry());
		printer.print(id);
		std::string str = ss.str();

		return Py_BuildValue("s", str.c_str());
	}
}

static PyObject* emb_storeInteger(PyObject *self, PyObject *args) {
	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.storeInteger: Expect exactly one parameter");
	dlvhex::ID id = dlvhex::ID::termFromInteger(PyInt_AsLong(PyTuple_GetItem(args, 0)));
	return Py_BuildValue("l", IDToLong(id));
}

static PyObject* emb_storeString(PyObject *self, PyObject *args) {
	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.storeString: Expect exactly one parameter");
	dlvhex::ID id = emb_ctx->registry()->storeConstantTerm(PyString_AsString(PyTuple_GetItem(args, 0)));
	return Py_BuildValue("l", IDToLong(id));
}

static PyObject* emb_storeAtom(PyObject *self, PyObject *args) {

	DBGLOG(DBG, "Storing atom of size " << PyTuple_Size(args));
	OrdinaryAtom atom(dlvhex::ID::MAINKIND_ATOM | dlvhex::ID::SUBKIND_ATOM_ORDINARYG);
	for (int i = 0; i < PyTuple_Size(args); ++i){
		atom.tuple.push_back(longToID(PyInt_AsLong(PyTuple_GetItem(args, i))));
	}

	dlvhex::ID id = emb_ctx->registry()->storeOrdinaryGAtom(atom);
	return Py_BuildValue("l", IDToLong(id));
}

static PyObject* emb_negate(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.negate: Expect exactly one parameter");
	dlvhex::ID id = longToID(PyInt_AsLong(PyTuple_GetItem(args, 0)));
	if (!id.isLiteral()) throw PluginError("dlvhex.negate: Can only negate literal IDs");
	id.kind ^= dlvhex::ID::NAF_MASK;
	return Py_BuildValue("l", IDToLong(id));
}

static PyObject* emb_learn(PyObject *self, PyObject *args) {

	if (!!emb_nogoods){
		Nogood ng;
		for (int i = 0; i < PyTuple_Size(args); ++i){
			dlvhex::ID id = longToID(PyInt_AsLong(PyTuple_GetItem(args, i)));
			if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.learn: Parameters must be positive or negated atom IDs");
			ng.insert(NogoodContainer::createLiteral(id));
		}
		emb_nogoods->addNogood(ng);

		return Py_BuildValue("i", 1);
	}else{
		return Py_BuildValue("i", 0);
	}
}

static PyObject* emb_getOutputAtom(PyObject *self, PyObject *args) {

	Tuple outputTuple;
	for (int i = 0; i < PyTuple_Size(args); ++i){
		dlvhex::ID id = longToID(PyInt_AsLong(PyTuple_GetItem(args, i)));
		if (!id.isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
		outputTuple.push_back(id);
	}
	dlvhex::ID id = ExternalLearningHelper::getOutputAtom(*emb_query, outputTuple, true);

	return Py_BuildValue("l", IDToLong(id));
}

static PyObject* emb_output(PyObject *self, PyObject *args) {

	Tuple outputTuple;
	for (int i = 0; i < PyTuple_Size(args); ++i){
		dlvhex::ID id = longToID(PyInt_AsLong(PyTuple_GetItem(args, i)));
		if (!id.isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
		outputTuple.push_back(id);
	}
	emb_answer->get().push_back(outputTuple);

	return Py_BuildValue("i", 0);
}

static PyObject* emb_outputValues(PyObject *self, PyObject *args) {

	Tuple outputTuple;
	for (int i = 0; i < PyTuple_Size(args); ++i){
		// try to store as integer
		int intv = PyInt_AsLong(PyTuple_GetItem(args, i));
		if (!PyErr_Occurred()){
			outputTuple.push_back(dlvhex::ID::termFromInteger(intv));
		}else{
			// store as string
			outputTuple.push_back(emb_ctx->registry()->storeConstantTerm(PyString_AsString(PyTuple_GetItem(args, i))));
		}
	}
	emb_answer->get().push_back(outputTuple);

	return Py_BuildValue("i", 0);
}

static PyObject* emb_getInputAtoms(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 0) throw PluginError("dlvhex.getInputAtoms: Expect no parameter");
	bm::bvector<>::enumerator en = emb_query->predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->predicateInputMask->getStorage().end();
	long i = 0;
	PyObject *pyIntr = PyTuple_New(emb_query->predicateInputMask->getStorage().count());
	while (en < en_end){
		PyTuple_SetItem(pyIntr, i, PyInt_FromLong(IDToLong(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en))));
		i++;
		en++;
	}

	return pyIntr;
}

static PyObject* emb_getInputAtom(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.getInputAtom: Expect exactly one parameter");
	long index = PyInt_AsLong(PyTuple_GetItem(args, 0));
	if (index < 0 || index >= emb_query->predicateInputMask->getStorage().count()) throw PluginError("dlvhex.getInputAtom: index out of bounds");
	bm::bvector<>::enumerator en = emb_query->predicateInputMask->getStorage().first();
	for (int i = 0; i < index; i++, en++);
	return Py_BuildValue("l", IDToLong(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en)));
}

static PyObject* emb_getInputAtomCount(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 0) throw PluginError("dlvhex.getInputAtomCount: Expect no parameter");
	return Py_BuildValue("l", emb_query->predicateInputMask->getStorage().count());
}

static PyObject* emb_isAssigned(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.isAssigned: Expect exactly one parameter");
	if (!!emb_query->assigned || emb_query->assigned->getFact(longToID(PyInt_AsLong(PyTuple_GetItem(args, 0))).address)) return Py_BuildValue("i", 1);
	else return Py_BuildValue("i", 0);
}

static PyObject* emb_isTrue(PyObject *self, PyObject *args) {

	if (PyTuple_Size(args) != 1) throw PluginError("dlvhex.isTrue: Expect exactly one parameter");
	if (emb_query->interpretation->getFact(longToID(PyInt_AsLong(PyTuple_GetItem(args, 0))).address)) return Py_BuildValue("i", 1);
	else return Py_BuildValue("i", 0);
}

PyMethodDef EmbMethods[] = {
	{"getTuple", emb_getTuple, METH_VARARGS, "Return the IDs of the elements of a dlvhex atom."},
	{"getTupleValues", emb_getTupleValues, METH_VARARGS, "Return the values of the elements of a dlvhex atom."},
	{"getValue", emb_getValue, METH_VARARGS, "Return the value of an atom or term ID)."},
	{"storeString", emb_storeString, METH_VARARGS, "Stores a string as dlvhex object."},
	{"storeInteger", emb_storeInteger, METH_VARARGS, "Stores an integer as dlvhex object."},
	{"storeAtom", emb_storeAtom, METH_VARARGS, "Transforms a sequence of terms into a dlvhex atom."},
	{"negate", emb_negate, METH_VARARGS, "Negates an atom ID."},
	{"learn", emb_learn, METH_VARARGS, "Learns a nogood."},
	{"getOutputAtom", emb_getOutputAtom, METH_VARARGS, "Constructs an output atom from term IDs (for learning purposes)."},
	{"output", emb_output, METH_VARARGS, "Adds a ground atom represented by an ID to the external source output."},
	{"outputValues", emb_outputValues, METH_VARARGS, "Adds a ground atom to the external source output."},
	{"getInputAtoms", emb_getInputAtoms, METH_VARARGS, "Returns a tuple of all input atoms to this external atom."},
//	{"getInputAtom", emb_getInputAtom, METH_VARARGS, "Returns the ID of an input atom with a given index."},
	{"getInputAtomCount", emb_getInputAtomCount, METH_VARARGS, "Returns the number of total input atoms (not the number of true input atoms!)."},
	{"isAssigned", emb_isAssigned, METH_VARARGS, "Checks if an input atom is assigned."},
	{"isTrue", emb_isTrue, METH_VARARGS, "Checks if an input atom is assigned to true."},
	{NULL, NULL, 0, NULL}
};

}

class PythonAtom : public PluginAtom
{
	private:
		PyObject *pName, *pModule, *pFunc;

	public:
		PythonAtom(ProgramCtx& ctx, std::string module, std::string functionName, std::vector<InputType> inputParameters, int outputArity)
			: PluginAtom(functionName.c_str(), false), pModule(pModule)
		{
			BOOST_FOREACH (InputType type, inputParameters){
				switch (type){
					case CONSTANT: addInputConstant(); break;
					case PREDICATE: addInputPredicate(); break;
					case TUPLE: addInputTuple(); break;
				}
			}		
			setOutputArity(outputArity);

			pName = PyString_FromString(module.c_str());
			pModule = PyImport_Import(pName);
			pFunc = PyObject_GetAttrString(pModule, functionName.c_str());
			if (!pFunc) throw PluginError("Python method \"" + functionName + "\" could not be loaded");

			PyObject *m = Py_InitModule("dlvhex", PythonAPI::EmbMethods);
		}

		virtual ~PythonAtom(){
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			assert (false);
		}

		virtual void
		retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods) throw (PluginError)
		{
			PyObject *pArgs, *pValue, *pyIntr;

			// pass interpretation as first argument to Python method
			bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
			bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
			long i = 0;
			pyIntr = PyTuple_New(query.interpretation->getStorage().count());
			while (en < en_end){
				PyTuple_SetItem(pyIntr, i, PyInt_FromLong(IDToLong(query.interpretation->getRegistry()->ogatoms.getIDByAddress(*en))));
				i++;
				en++;
			}
			int vdim[] = { i };
			pArgs = PyTuple_New(query.input.size());
			for (int i = 0; i < query.input.size(); ++i){
				PyTuple_SetItem(pArgs, i, PyInt_FromLong(IDToLong(query.input[i])));
			}

			PythonAPI::emb_query = &query;
			PythonAPI::emb_answer = &answer;
			PythonAPI::emb_nogoods = nogoods;

			// call Python method
			PyObject *res = PyObject_CallObject(pFunc, pArgs);

			PythonAPI::emb_query = NULL;
			PythonAPI::emb_answer = NULL;
			PythonAPI::emb_nogoods.reset();

			if (!res){
				PyErr_Print();
				throw PluginError("Python script failed");
			}
		}
};

std::vector<PluginAtomPtr> PythonPlugin::createAtoms(ProgramCtx& ctx) const{

	std::vector<PluginAtomPtr> ret;

	PythonAPI::emb_ctx = &ctx;

	// we have to do the program rewriting already here because it creates some side information that we need
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	// include plugin dirs in the Python path
	std::string oldenv_ld;
	const char *envld = ::getenv("PYTHONPATH");
	if (envld) {
		oldenv_ld = envld;
		unsetenv("PATHONPATH");
	}
	setenv("PYTHONPATH", (ctx.config.getStringOption("PluginDirs") + ":" + oldenv_ld).c_str(), 1);

	// load Python plugins
	BOOST_FOREACH (std::string script, ctxdata.pythonScripts){

		PyObject *pModule, *pName, *pDict, *pFunc;
		PyObject *pArgs, *pValue;

		Py_Initialize();
		PyObject *m = Py_InitModule("dlvhex", PythonAPI::EmbMethods);
		DBGLOG(DBG, "PythonPlugin: Loading script \"" << script << "\"");
		pName = PyString_FromString(script.c_str());
		pModule = PyImport_Import(pName);
		Py_DECREF(pName);

		if (pModule != NULL) {
			DBGLOG(DBG, "PythonPlugin: Loading atoms from script \"" << script << "\"");
			pFunc = PyObject_GetAttrString(pModule, "register");

			/* pFunc is a new reference */
			if (pFunc && PyCallable_Check(pFunc)) {
				pArgs = PyTuple_New(0);
				pValue = PyObject_CallObject(pFunc, pArgs);
				Py_DECREF(pArgs);
				if (pValue != NULL) {
					int atomCount = PyTuple_Size(pValue);
					DBGLOG(DBG, "Found " << atomCount << " external atoms");
					for (int i = 0; i < atomCount; ++i){
						PyObject *atom = PyTuple_GetItem(pValue, i);
						if ((PyTuple_Size(atom) == 0) || (i == atomCount - 1)){
							if ((PyTuple_Size(atom) == 0) && (i == atomCount - 1)) break;
							else throw PluginError("Empty tuple must occur at the end of the atom list");
						}
						if (PyTuple_Size(atom) < 2) throw PluginError("Python plugin, register method: External atoms must specify at least 1. a name and 2. the output arity");
						std::string atomName = PyString_AsString(PyTuple_GetItem(atom, 0));
						DBGLOG(DBG, "Loading external atom " << atomName);

						std::vector<PluginAtom::InputType> inputParameters;
						int outputArity = PyInt_AsLong(PyTuple_GetItem(atom, PyTuple_Size(atom) - 1));
						for (int p = 1; p < PyTuple_Size(atom) - 1; ++p){
							if (std::string(PyString_AsString(PyTuple_GetItem(atom, p))) == std::string("c")) inputParameters.push_back(PluginAtom::CONSTANT);
							else if (std::string(PyString_AsString(PyTuple_GetItem(atom, p))) == std::string("p")) inputParameters.push_back(PluginAtom::PREDICATE);
							else if (std::string(PyString_AsString(PyTuple_GetItem(atom, p))) == std::string("t")) inputParameters.push_back(PluginAtom::TUPLE);
							else throw PluginError("Unknown parameter type: \"" + std::string(PyString_AsString(PyTuple_GetItem(atom, p))) + "\"");
						}

						// return smart pointer with deleter (i.e., delete code compiled into this plugin)
						ret.push_back(PluginAtomPtr(new PythonAtom(ctx, script, atomName, inputParameters, outputArity), PluginPtrDeleter<PluginAtom>()));
					}

					Py_DECREF(pValue);
				}else{
					Py_DECREF(pFunc);
					PyErr_Print();
				}
			}else{
				if (PyErr_Occurred()) PyErr_Print();
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
		}else{
		        throw GeneralError("Cannot find script \"" + script + "\". Make sure that environment variable PYTHONPATH is set appropriately.");
		}
	}

	return ret;
}

DLVHEX_NAMESPACE_END

#endif

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
PythonPlugin thePythonPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE thePythonPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End: