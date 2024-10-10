#ifndef __PROLOG_MICRO_BRAIN_H__
#define __PROLOG_MICRO_BRAIN_H__

#include <vector>
#include <string>
#include <set>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>

#include <time.h>

using namespace std;

#include "elements.h"

extern bool fast_memory_manager;
extern unsigned int mem_block_size;

extern const char * STD_INPUT;
extern const char * STD_OUTPUT;

const int once_flag = 0x1;
const int call_flag = 0x2;

unsigned long long getTotalSystemMemory();
void * __alloc(size_t size);
void __free(void * ptr);

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#include <dlfcn.h>

typedef void * HMODULE;

HMODULE LoadLibrary(wchar_t * _fname) {
	return dlopen(wstring_to_utf8(_fname).c_str(), RTLD_LAZY);
}

void * GetProcAddress(HMODULE handle, char * fname) {
	return dlsym(handle, fname);
}

void FreeLibrary(HMODULE handle) {
	dlclose(handle);
}
#else
#include <Windows.h>
#endif

template<class T> class stack_container : public vector<T>{
public:
	virtual void push_back(const T & v) {
		if (this->capacity() == this->size())
			this->reserve((int)(1.5*this->size()));
		vector<T>::push_back(v);
	}

	virtual void push(const T & v) { this->push_back(v); }
	virtual void pop() { this->pop_back(); }
	virtual T top() { return this->back(); }
};

class interpreter;
class frame_item;
class predicate;
class clause;
class predicate_item;
class predicate_item_user;
class generated_vars;
class value;
class term;
class tthread;
class tframe_item;

typedef struct {
	char* _name;
	value* ptr;
} mapper;

class context {
public:
	context(int RESERVE, context * parent, tframe_item * tframe, predicate_item * starting, predicate_item * ending, interpreter * prolog) {
		this->parent = parent;

		CALLS.reserve(RESERVE);
		FRAMES.reserve(RESERVE);
		CTXS.reserve(RESERVE);
		NEGS.reserve(RESERVE);
		_FLAGS.reserve(RESERVE);
		PARENT_CALLS.reserve(RESERVE);
		PARENT_CALL_VARIANT.reserve(RESERVE);
		CLAUSES.reserve(RESERVE);
		// FLAGS;
		LEVELS.reserve(RESERVE);
		TRACE.reserve(RESERVE);
		TRACEARGS.reserve(RESERVE);
		TRACERS.reserve(RESERVE);
		ptrTRACE.reserve(RESERVE);

		THR = NULL;
		FRAME = tframe;
		this->starting = starting;
		this->ending = ending;
		this->prolog = prolog;
	}

	virtual ~context() {
		for (context* C : CONTEXTS)
			delete C;
		delete THR;
		delete FRAME;
	}

	bool forked() {
		bool locked = pages_mutex.try_lock();
		context* C = this;
		while (C && !C->THR)
			C = C->parent;
		bool result = C && C->THR;
		if (locked) pages_mutex.unlock();
		return result;
	}

	context* parent;
	interpreter* prolog;
	predicate_item* starting;
	predicate_item* ending;

	vector<context*> CONTEXTS;
	tframe_item* FRAME;
	tthread* THR;

	stack_container<predicate_item*> CALLS;
	stack_container<frame_item*> FRAMES;
	stack_container<context*> CTXS;
	stack_container<bool> NEGS;
	stack_container<int> _FLAGS;
	stack_container<predicate_item_user*> PARENT_CALLS;
	stack_container<int> PARENT_CALL_VARIANT;
	stack_container<clause*> CLAUSES;
	std::list<int> FLAGS;
	stack_container<int> LEVELS;
	stack_container<generated_vars*> TRACE;
	stack_container<vector<value*>**> TRACEARGS;
	stack_container<predicate_item*> TRACERS;
	stack_container<int> ptrTRACE;

	std::mutex pages_mutex;

	virtual context* add_page(tframe_item* f, predicate_item* starting, predicate_item* ending, interpreter* prolog);

	virtual bool join(int K, frame_item* f, interpreter* INTRP);
};

class interpreter {
private:
	predicate_item_user * starter;
	HMODULE xpathInductLib;
	char env[65536 * 4];
	bool xpath_compiled;
	bool xpath_loaded;
public:
	string CLASSES_ROOT;
	string INDUCT_MODE;

	map<string, predicate *> PREDICATES;
	std::mutex DBLOCK;
	map< string, vector<term *> *> DB;
	map< string, set<int> *> DBIndicators;
	std::mutex GLOCK;
	map<string, value *> GVars;

	context* CONTEXT;

	string current_output;
	string current_input;
	string out_buf;

	basic_istream<char> * ins;
	basic_ostream<char> * outs;

	int file_num;
	map<int, std::basic_fstream<char> > files;

	interpreter(const string & fname, const string & starter_name);
	~interpreter();

	bool XPathCompiled() {
		return xpath_compiled;
	}
	bool XPathLoaded() {
		return xpath_loaded;
	}
	void SetXPathCompiled(bool v) {
		xpath_compiled = v;
	}
	void SetXPathLoaded(bool v) {
		xpath_loaded = v;
	}

	bool LoadXPathing() {
		bool result = false;
		if (!xpathInductLib) {
			xpathInductLib = LoadLibrary(
#ifdef __linux__
				L"./libxpathInduct.so"
#else
				L"xpathInduct.dll"
#endif
				);
			if (xpathInductLib) {
				XPathInduct = (XPathInductF)GetProcAddress(xpathInductLib, "XPathInduct");
				CompileXPathing = (CompileXPathingF)GetProcAddress(xpathInductLib, "CompileXPathing");
				SetInterval = (SetIntervalF)GetProcAddress(xpathInductLib, "SetInterval");
				ClearRestrictions = (ClearRestrictionsF)GetProcAddress(xpathInductLib, "ClearRestrictions");
				ClearRuler = (ClearRulerF)GetProcAddress(xpathInductLib, "ClearRuler");
				SetDeduceLogFile = (SetDeduceLogFileF)GetProcAddress(xpathInductLib, "SetDeduceLogFile");
				CreateDOMContact = (CreateDOMContactF)GetProcAddress(xpathInductLib, "CreateDOMContact");
				GetMSG = (GetMSGF)GetProcAddress(xpathInductLib, "GetMSG");
				result = true;
			}
		}
		return result;
	}

	void UnloadXPathing() {
		if (xpathInductLib)
			FreeLibrary(xpathInductLib);
		xpathInductLib = 0;
	}

	char * ENV() {
		return env;
	}

	string open_file(const string & fname, const string & mode);
	void close_file(const string & obj);
	std::basic_fstream<char> & get_file(const string & obj, int & fn);

	void block_process(context * CNT, bool clear_flag, bool cut_flag, predicate_item * frontier);

	double evaluate(context* CTX, frame_item * ff, const string & expression, int & p);

	bool check_consistency(set<string> & dynamic_prefixes);

	void add_std_body(string & body);

	vector<value *> * accept(context* CTX, frame_item * ff, predicate_item * current);
	vector<value *> * accept(context* CTX, frame_item * ff, clause * current);
	bool retrieve(context* CTX, frame_item * ff, clause * current, vector<value *> * vals, bool escape_vars);
	bool retrieve(context* CTX, frame_item * ff, predicate_item * current, vector<value *> * vals, bool escape_vars);
	bool process(context* CNT, bool neg, clause * this_clause_call, predicate_item * p, frame_item * f, vector<value *> ** positional_vals);

	void consult(const string & fname, bool renew);
	void bind();
	predicate_item_user * load_program(const string & fname, const string & starter_name);

	value * parse(context* CTX, bool exit_on_error, bool parse_complex_terms, frame_item * ff, const string & s, int & p);
	void parse_program(vector<string> & renew, string & s);
	void parse_clause(context* CTX, vector<string> & renew, frame_item * ff, string & s, int & p);
	bool unify(context* CTX, frame_item * ff, value * from, value * to);

	void run();
	bool loaded();
};

class value {
protected:
	int refs;
public:
	value() { refs = 1; }

	virtual void use() { refs++; }
	virtual void free() { refs--; if (refs == 0) delete this; }

	virtual value * fill(context * CTX, frame_item * vars) = 0;
	virtual value * copy(context * CTX, frame_item * f, int unwind = 0) = 0;
	virtual value * const_copy(context * CTX, frame_item * f) {
		if (this->defined()) {
			this->use();
			return this;
		} else
			return copy(CTX, f);
	}
	virtual bool unify(context * CTX, frame_item * ff, value * from) = 0;
	virtual bool defined() = 0;

	virtual string to_str(bool simple = false) = 0;

	virtual void escape_vars(context * CTX, frame_item * ff) = 0;

	virtual string make_str() {
		return to_str();
	}

	virtual string export_str(bool simple = false, bool double_slashes = true) {
		return to_str();
	}

	virtual void write(basic_ostream<char> * outs, bool simple = false) {
		(*outs) << to_str(simple);
	}

	void * operator new (size_t size){
		if (fast_memory_manager)
		return __alloc(size);
		else {
			void *ptr = ::operator new(size);
//			cout<<"new:"<<ptr<<endl;
			return ptr;
		}
	}

	void operator delete (void * ptr) {
//	cout << "del:" << ptr << endl;
	if (fast_memory_manager)
		__free(ptr);
	else
		::operator delete(ptr);
	}
};

class stack_values : public stack_container<value *> {
public:
	virtual void free() {
		for (int i = 0; i < this->size(); i++)
			at(i)->free();
	}
};

class frame_item {
protected:
	friend class tframe_item;

	vector<char> names;

	vector<mapper> vars;
public:
	frame_item() {
		names.reserve(32);
		vars.reserve(8);
	}

	virtual ~frame_item() {
		int it = 0;
		while (it < vars.size()) {
			if (vars[it].ptr)
				vars[it].ptr->free();
			it++;
		}
	}

	virtual void add_local_names(std::set<string>& s) {
		for (mapper& m : vars)
			if (m._name[0] != '$')
				s.insert(m._name);
	}

	virtual void sync(context * CTX, frame_item * other) {
		int it = 0;
		while (it < other->vars.size()) {
			char * itc = other->vars[it]._name;
			set(CTX, itc, other->vars[it].ptr);
			it++;
		}
	}

	virtual frame_item * copy(context * CTX) {
		frame_item * result = new frame_item();
		result->names = names;
		long long d = result->names.size() == 0 ? 0 : &result->names[0] - &names[0];
		result->vars = vars;
		for (mapper & m : result->vars) {
			m.ptr = m.ptr ? m.ptr->const_copy(CTX, this) : NULL;
			m._name += d;
		}
		return result;
	}

	virtual tframe_item* tcopy(context* CTX, interpreter * INTRP);

	int get_size() { return vars.size(); }

	value * at(int i) {
		return vars[i].ptr;
	}

	char * atn(int i) {
		return vars[i]._name;
	}

	int find(const char * what, bool & found) {
		found = false;

		if (vars.size() == 0)
			return 0;
		if (strcmp(what, vars[0]._name) < 0)
			return 0;
		if (strcmp(what, vars.back()._name) > 0)
			return vars.size();

		int a = 0;
		int b = vars.size() - 1;
		while (a < b) {
			int c = (a + b) / 2;
			int r = strcmp(what, vars[c]._name);
			if (r == 0) {
				found = true;
				return c;
			}
			if (r < 0)
				b = c;
			else
				a = c + 1;
		}
		if (strcmp(what, vars[b]._name) == 0)
			found = true;
		return b;
	}

	void escape(const char * name, char prepend) {
		bool found = false;
		int it = find(name, found);
		if (found) {
			char * oldp = &names[0];
			char * s = atn(it);
			names.insert(names.begin() + (s - oldp), prepend);
			char * newp = &names[0];
			char * news = newp + (s - oldp);
			for (int i = 0; i < vars.size(); i++)
				if (i != it)
					if (atn(i) > s)
						vars[i]._name += (newp - oldp) + 1;
					else
						vars[i]._name += (newp - oldp);
			value * v = vars[it].ptr;
			vars.erase(vars.begin() + it);

			found = false;
			it = find(news, found);
			if (found) {
				cout << "Internal FRAME collision : " << news << " already exists and can't be escaped?!" << endl;
				exit(4000);
			}
			else {
				mapper m = { news, v };
				vars.insert(vars.begin() + it, m);
			}
		}
	}

	virtual void set(context * CTX, const char * name, value * v) {
		bool found = false;
		int it = find(name, found);
		if (found) {
			vars[it].ptr->free();
			vars[it].ptr = v ? v->copy(CTX, this) : NULL;
		}
		else {
			char * oldp = names.size() == 0 ? NULL : &names[0];
			int oldn = names.size();
			int n = strlen(name);
			names.resize(oldn + n + 1);
			char * newp = &names[0];
			for (int i = 0; i < vars.size(); i++)
				vars[i]._name += newp - oldp;
			char * _name = newp + oldn;
			strcpy(_name, name);
			mapper m = { _name, v ? v->copy(CTX, this) : NULL };
			vars.insert(vars.begin() + it, m);
		}
	}

	virtual value * get(context * CTX, const char * name, int unwind = 0) {
		bool found = false;
		int it = find(name, found);
		if (found)
			return vars[it].ptr;
		else if (unwind && name[0] == '$') {
			return get(CTX, &name[1], unwind-1);
		}
		else
			return NULL;
	}
};

class tthread : public std::thread {
	int _id;
	volatile std::atomic<bool> stopped;
	volatile std::atomic_flag terminated; // If needs to be terminated
	volatile std::atomic<bool> result; // Logical result

	context* CONTEXT;
public:
	tthread(int _id, context * CTX) : std::thread(&tthread::body, this) {
		this->_id = _id;
		CONTEXT = CTX;
		stopped.store(false);
		terminated.clear();
		result = false;
	}
	virtual ~tthread() { }

	void body();

	virtual context* get_context() { return CONTEXT; }

	virtual int get_nthread() { return _id; }
	virtual bool is_stopped() { return stopped.load(); }
	virtual void set_stopped(bool v) { stopped.store(v); }
	virtual bool is_terminated() {
		bool state = terminated.test_and_set();
		if (!state) terminated.clear();
		return state;
	}
	virtual void set_terminated(bool v) {
		if (v) terminated.test_and_set();
		else terminated.clear();
	}
	virtual bool get_result() {
		return result.load();
	}
	virtual void set_result(bool v) {
		result.store(v);
	}
};

context* context::add_page(tframe_item* f, predicate_item * starting, predicate_item * ending, interpreter * prolog) {
	std::unique_lock<std::mutex> plock(pages_mutex);

	context* result = new context(100, this, f, starting, ending, prolog);

	CONTEXTS.push_back(result);

	result->THR = new tthread(CONTEXTS.size() - 1, result);

	plock.unlock();

	return result;
}

class tframe_item : public frame_item {
	friend class frame_item;

	vector<clock_t> last_reads;
	vector<clock_t> first_writes;

	std::mutex mutex;
public:
	tframe_item() : frame_item() { }

	virtual void set(context * CTX, const char* name, value* v) {
		bool locked = mutex.try_lock();
		bool found = false;
		int it = find(name, found);
		if (!found) {
			char* oldp = names.size() == 0 ? NULL : &names[0];
			int oldn = names.size();
			int n = strlen(name);
			names.resize(oldn + n + 1);
			char* newp = &names[0];
			for (int i = 0; i < vars.size(); i++)
				vars[i]._name += newp - oldp;
			char* _name = newp + oldn;
			strcpy(_name, name);
			mapper m = { _name, v ? v->copy(CTX, this) : NULL };
			vars.insert(vars.begin() + it, m);
			last_reads.insert(last_reads.begin() + it, 0);
			first_writes.insert(first_writes.begin() + it, 0);
		}
		else {
			if (vars[it].ptr)
				vars[it].ptr->free();
			vars[it].ptr = v ? v->copy(CTX, this) : NULL;
		}
		if (first_writes[it] == 0)
			first_writes[it] = clock();
		if (locked) mutex.unlock();
	}

	virtual value* get(context * CTX, const char* name, int unwind = 0) {
		bool locked = mutex.try_lock();
		value* result = NULL;
		bool found = false;
		int it = find(name, found);
		if (found) {
			result = vars[it].ptr;
			last_reads[it] = clock();
		}
		else if (unwind && name[0] == '$') {
			result = frame_item::get(CTX, &name[1], unwind - 1);
		}
		if (locked) mutex.unlock();
		return result;
	}

	virtual clock_t first_write(const std::string& vname) {
		bool found = false;
		int it = find(vname.c_str(), found);
		if (found)
			return first_writes[it];
		else
			return 0;
	}

	virtual clock_t last_read(const std::string& vname) {
		bool found = false;
		int it = find(vname.c_str(), found);
		if (found)
			return last_reads[it];
		else
			return 0;
	}

	virtual void sync(context* CTX, frame_item* other) {
		tframe_item* _other = dynamic_cast<tframe_item*>(other);
		if (!_other) {
			frame_item::sync(CTX, other);
			return;
		}
		int it = 0;
		while (it < _other->vars.size()) {
			bool f;
			char* itc = _other->vars[it]._name;
			set(CTX, itc, _other->vars[it].ptr);
			int _it = find(itc, f);
			if (f)
				first_writes[_it] = _other->first_writes[it];
			else {
				cout << "Internal ERROR : can't sync tframe_item : var '" << itc << "'" << endl;
				exit(-60);
			}
			it++;
		}
	}

	virtual frame_item* copy(context* CTX) {
		frame_item* result = new tframe_item();
		result->names = names;
		long long d = result->names.size() == 0 ? 0 : &result->names[0] - &names[0];
		result->vars = vars;
		for (mapper& m : result->vars) {
			m.ptr = m.ptr ? m.ptr->const_copy(CTX, this) : NULL;
			m._name += d;
		}
		((tframe_item*)result)->first_writes = first_writes;
		((tframe_item*)result)->last_reads = last_reads;
		return result;
	}

	virtual tframe_item* tcopy(context* CTX, interpreter* INTRP) {
		return dynamic_cast<tframe_item *>(copy(CTX));
	}
};

tframe_item* frame_item::tcopy(context* CTX, interpreter* INTRP) {
	tframe_item* result = new tframe_item();
	result->names = names;
	long long d = result->names.size() == 0 ? 0 : &result->names[0] - &names[0];
	result->vars = vars;
	for (mapper& m : result->vars) {
		m.ptr = m.ptr ? m.ptr->const_copy(CTX, this) : NULL;
		m._name += d;
	}
	if (CTX && (CTX->THR || CTX->forked())) {
		bool locked = INTRP->GLOCK.try_lock();
		std::map<std::string, value*>::iterator it = INTRP->GVars.begin();
		result->first_writes = vector<clock_t>(vars.size(), 0);
		result->last_reads = vector<clock_t>(vars.size(), 0);
		while (it != INTRP->GVars.end()) {
			if (it->first.length() && it->first[0] != '&') {
				string new_name = it->first;
				new_name.insert(0, 1, '*');
				result->set(CTX, new_name.c_str(), it->second ? it->second->copy(CTX, this) : NULL);
			}
			it++;
		}
		if (locked) INTRP->GLOCK.unlock();
	}
	result->first_writes = vector<clock_t>(result->vars.size(), 0);
	result->last_reads = vector<clock_t>(result->vars.size(), 0);
	return result;
}

bool context::join(int K, frame_item* f, interpreter * INTRP) {
	std::unique_lock<std::mutex> lock(pages_mutex);
	if (CONTEXTS.size() == 0) {
		lock.unlock();
		return true;
	}

	context* C = this;
	while (C && !C->THR)
		C = C->parent;
	bool forked = C && C->THR;

	int joined = 0;
	int success = 0;
	vector<bool> joineds(CONTEXTS.size(), false);
	std::set<string> names;
	do {
		vector<bool> stoppeds = joineds;
		int stopped = std::count(stoppeds.begin(), stoppeds.end(), true);
		while (stopped < CONTEXTS.size()) {
			for (int i = 0; i < CONTEXTS.size(); i++)
				if (!stoppeds[i] && CONTEXTS[i]->THR->is_stopped()) {
					stoppeds[i] = true;
					CONTEXTS[i]->FRAME->add_local_names(names);
					stopped++;
				}
			std::this_thread::yield();
		}
		// In not joineds search for winners
		vector<vector<int>> M(CONTEXTS.size(), vector<int>(CONTEXTS.size(), 0));
		for (const string& vname : names) {
			// Conflict Matrix
			for (int i = 0; i < CONTEXTS.size(); i++)
				for (int j = i + 1; j < CONTEXTS.size(); j++)
					if (!joineds[i] && !joineds[j]) {
						clock_t fwi = CONTEXTS[i]->FRAME->first_write(vname);
						clock_t fwj = CONTEXTS[j]->FRAME->first_write(vname);
						clock_t lri = CONTEXTS[i]->FRAME->last_read(vname);
						clock_t lrj = CONTEXTS[j]->FRAME->last_read(vname);
						if (fwi || fwj) {
							if (fwi && fwj)
								M[i][j]++;
							else if (fwi) {
								if (lrj >= fwi)
									M[i][j]++;
							}
							else {
								if (lri >= fwj)
									M[i][j]++;
							}
						}
					}
		}
		// Decide
		int first_winner = -1;
		std::set<int> other_winners;
		for (int i = 0; i < CONTEXTS.size(); i++)
			if (!joineds[i]) {
				std::set<int> parallels;
				for (int j = 0; j < i; j++) {
					if (!joineds[j] && M[j][i] == 0)
						parallels.insert(j);
				}
				for (int j = i + 1; j < CONTEXTS.size(); j++) {
					if (!joineds[j] && M[i][j] == 0)
						parallels.insert(j);
				}
				if (parallels.size() > other_winners.size()) {
					other_winners = parallels;
					first_winner = i;
				}
			}
		if (first_winner < 0) {
			for (int i = 0; first_winner < 0 && i < CONTEXTS.size(); i++)
				if (!joineds[i])
					first_winner = i;
		}
		// Set vars by threads with non-zero first_writes
		for (const string& vname : names) {
			int winner = -1;
			if (CONTEXTS[first_winner]->FRAME->first_write(vname))
				winner = first_winner;
			else {
				for (int i : other_winners)
					if (CONTEXTS[i]->FRAME->first_write(vname)) {
						winner = i;
						break;
					}
			}
			if (winner >= 0 && CONTEXTS[winner]->THR->get_result()) {
				value* old = f->get(this, vname.c_str());
				value* cur = CONTEXTS[winner]->FRAME->get(CONTEXTS[winner], vname.c_str());
				if (cur)
					if (old && (vname.length() == 0 || vname[0] != '*')) {
						if (!old->unify(this, f, cur))
							CONTEXTS[winner]->THR->set_result(false);
					}
					else
						f->set(this, vname.c_str(), cur);
			}
		}
		// Join first_winner && other_winners
		other_winners.insert(first_winner);
		for (int i = 0; i < CONTEXTS.size(); i++)
			if (!joineds[i]) {
				if (other_winners.find(i) == other_winners.end()) {
					delete CONTEXTS[i]->FRAME;
					CONTEXTS[i]->FRAME = f->tcopy(this, INTRP);
					CONTEXTS[i]->THR->set_stopped(false);
				}
				else {
					CONTEXTS[i]->THR->set_terminated(true);
					CONTEXTS[i]->THR->set_stopped(false);
					CONTEXTS[i]->THR->join();
					joineds[i] = true;
					if (CONTEXTS[i]->THR->get_result())
						success++;
					joined++;
				}
			}
	} while (joined < CONTEXTS.size());

	bool result = K < 0 && success == CONTEXTS.size() || K >= 0 && success >= K;

	prolog->GLOCK.lock();
	for (const string& vname : names) {
		value* v = f->get(this, vname.c_str());
		for (int i = 0; i < CONTEXTS.size(); i++)
			CONTEXTS[i]->FRAME->set(CONTEXTS[i], vname.c_str(), NULL);
		if (!forked && v && vname.length() && vname[0] == '*') {
			string src_name = vname.substr(1);
			std::map<std::string, value*>::iterator it = prolog->GVars.find(src_name);
			if (it != prolog->GVars.end()) {
				it->second->free();
			}
			prolog->GVars[src_name] = v->copy(this, f);
		}
	}
	prolog->GLOCK.unlock();

	for (context* C : CONTEXTS)
		delete C;
	CONTEXTS.clear();

	lock.unlock();
	return result;
}

class generated_vars : public vector<frame_item *> {
public:
	generated_vars() : vector<frame_item *>() { }
	generated_vars(int n, frame_item * filler) : vector<frame_item *>(n, filler) { }

	virtual bool has_variant(int i) { return i < size(); }
	virtual bool had_variants() { return size() != 0; }

	virtual void trunc_from(int k) {
		delete_from(k);
		this->resize(k);
	}

	virtual frame_item * get_next_variant(context * CTX, int i) { return at(i); }

	virtual void delete_from(int i) {
		for (int j = i; j < size(); j++)
			delete at(j);
	}
};

class clause {
	friend predicate_item;
private:
	predicate * parent;
	std::list<string> args;
	std::list<value *> _args;
	vector<predicate_item *> items;
	std::mutex mutex;
	bool forking;
public:
	clause(predicate * pp) : parent(pp), forking(false) { }
	~clause();

	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }

	virtual void bind();

	predicate * get_predicate() { return parent; }

	bool is_forking() { return forking; }
	void set_forking(bool v) {
		forking = v;
	}

	const std::list<string> * get_args() {
		return &args;
	}

	const std::list<value *> * _get_args() {
		return &_args;
	}

	void push_cashed_arg(value * v) {
		v->use();
		_args.push_back(v);
	}

	void set_args(const std::list<string> & other_args) {
		args = other_args;
	}

	void add_item(predicate_item * p) {
		items.push_back(p);
	}

	void add_arg(const string & a) {
		args.push_back(a);
	}

	virtual int num_items() { return items.size(); }
	virtual predicate_item * get_item(int i) { return items[i]; }
};

class predicate_item {
protected:
	int self_number;
	clause * parent;
	std::list<string> args;
	std::list<value *> _args;

	interpreter * prolog;

	bool neg;
	bool once;
	bool call;

	std::mutex mutex;
public:
	predicate_item(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : neg(_neg), once(_once), call(_call), self_number(num), parent(c), prolog(_prolog) { }
	virtual ~predicate_item() {
		for (value * v : _args)
			v->free();
	}

	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }

	virtual void bind() { }

	virtual const string get_id() = 0;

	bool is_negated() { return neg; }
	bool is_once() { return once; }
	bool is_call() { return call; }

	void make_once() { once = true; }
	void clear_negated() { neg = false; }

	const std::list<value *> * _get_args() {
		return &_args;
	}

	void push_cashed_arg(value * v) {
		v->use();
		_args.push_back(v);
	}

	void add_arg(const string & a) {
		args.push_back(a);
	}

	virtual predicate_item * get_next(int variant) {
		if (!is_last()) {
			return parent->items[self_number + 1];
		}
		else
			return NULL;
	}

	virtual frame_item * get_next_variant(context * CTX, frame_item * f, int & internal_variant, vector<value *> * positional_vals) { return NULL; }

	const std::list<string> * get_args() {
		return &args;
	}

	clause * get_parent() { return parent; }

	bool is_first() { return self_number == 0; }
	bool is_last() { return !parent || self_number == parent->items.size() - 1; }

	virtual generated_vars * generate_variants(context * CTX, frame_item * f, vector<value *> * & positional_vals) = 0;

	virtual bool execute(context * CTX, frame_item * f) {
		return true;
	}
};

class predicate {
	friend predicate_item;
	friend clause;
private:
	string name;
	vector<clause *> clauses;
	bool forking;
public:
	predicate(const string & _name) : name(_name), forking(false) { }
	~predicate();

	void bind() {
		for (clause * c : clauses)
			c->bind();
	}

	void add_clause(clause * c) {
		clauses.push_back(c);
	}

	bool is_forking() { return forking; }
	void set_forking(bool v) {
		forking = v;
	}
	bool is_forking_variant(int variant) {
		return clauses[variant]->is_forking();
	}

	virtual int num_clauses() { return clauses.size(); }
	virtual clause * get_clause(int i) { return clauses[i]; }
};

class predicate_item_user : public predicate_item {
private:
	string id;
	predicate * user_p;
public:
	predicate_item_user(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog, const string & _name) : predicate_item(_neg, _once, _call, num, c, _prolog), id(_name) {
		user_p = NULL;
	}

	virtual void bind() {
		if (prolog->PREDICATES.find(id) == prolog->PREDICATES.end())
			user_p = NULL;
		else
			user_p = prolog->PREDICATES[id];
	}

	bool is_dynamic() { return user_p == NULL; }

	bool is_forking_variant(int variant) { return user_p != NULL && user_p->is_forking_variant(variant); }

	predicate * get_user_predicate() { return user_p; }

	virtual const string get_id() { return id; }

	virtual generated_vars * generate_variants(context * CTX, frame_item * f, vector<value *> * & positional_vals);

	virtual bool processing(context * CONTEXT, bool line_neg, int variant, generated_vars * variants, vector<value *> ** positional_vals, frame_item * up_f, context * up_c);
};

void tthread::body() {
	set_stopped(false);
	do {
		terminated.clear();
		// Process
		predicate_item* first = CONTEXT->starting;
		vector<value*>* first_args = CONTEXT->prolog->accept(CONTEXT, CONTEXT->FRAME, first);
		set_result(CONTEXT->prolog->process(CONTEXT, false, first->get_parent(), first, CONTEXT->FRAME, &first_args));
		if (first_args) {
			for (int j = 0; j < first_args->size(); j++)
				first_args->at(j)->free();
			delete first_args;
		}

		set_stopped(true);
		while (is_stopped())
			std::this_thread::yield();
	} while (!terminated.test_and_set());
}

#endif
