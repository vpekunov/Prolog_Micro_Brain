// (C) V.V.Pekunov, Just For Fun :)

// g++ -o prolog_micro_brain prolog_micro_brain.cpp -O4 -std=c++0x -msse4

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <algorithm>
#include <functional>

#include <math.h>
#include <string.h>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif

void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s) - 1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

char * __ltoa(long long n, char s[], long long radix)
{
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;          /* make n positive */
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = n % radix + '0';   /* get next digit */
	} while ((n /= radix) > 0);     /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);

	return s;
}

#define __itoa __ltoa

const char * STD_INPUT = "#STD_INPUT";
const char * STD_OUTPUT = "#STD_OUTPUT";

/**/
bool built_flag = false;

using namespace std;

class frame_item;
class predicate;
class clause;
class predicate_item;
class predicate_item_user;
class generated_vars;
class value;
class term;

const int once_flag = 0x1;
const int call_flag = 0x2;

string escape(const string & s) {
	string result;

	int i = 0;
	while (i < s.length()) {
		if (s[i] == '\\') {
			result += s[i];
			result += s[i];
		}
		else
			result += s[i];
		i++;
	}

	return result;
}

string unescape(const string & s) {
	string result;

	int i = 0;
	while (i < s.length()) {
		if (i + 1 < s.length() && s[i] == '\\') {
			switch (s[i + 1]) {
			case 'r': result += "\r"; break;
			case 'n': result += "\n"; break;
			case 't': result += "\t"; break;
			case '0': result += "\0"; break;
			default: result += s[i + 1];
			}
			i++;
		}
		else
			result += s[i];
		i++;
	}

	return result;
}

template<class T> class stack_container : public vector<T>{
public:
	virtual void push_back(const T & v) {
		if (this->capacity() == this->size())
			this->reserve((int)(1.5*this->size()));
		vector<T>::push_back(v);
	}

	virtual void push(const T & v) { this->push_back(v); }
	virtual void pop() { this->pop_back(); }
	virtual T top() { return this->back();  }
};

const unsigned int mem_block_size = 1024 * 1024;

typedef struct {
	unsigned int available;
	unsigned int top;
	char mem[mem_block_size];
} mem_block;

typedef struct {
	int start_offs; // Negative if is freed
	unsigned int size;
	char mem[10];
} mem_header;

bool fast_memory_manager = true;

stack_container<mem_block *> used;
stack_container<mem_block *> freed;

void * __alloc(size_t size) {
	unsigned int occupied = size + (sizeof(int) + sizeof(unsigned int));
	
	char rest = occupied % 8;

	if (rest) {
		occupied += 8 - rest;
	}

	auto new_block = [&](mem_block * m)->void * {
		m->available = mem_block_size - occupied;
		m->top = occupied;
		mem_header * mm = (mem_header *)&m->mem;
		mm->start_offs = (int)((char *)mm - (char *)m);
		mm->size = occupied;

		return &mm->mem;
	};

	auto simple_alloc = [&](mem_block * m)->void * {
		mem_header * mm = (mem_header *)&m->mem[m->top];
		m->available -= occupied;
		m->top += occupied;
		mm->start_offs = (int)((char *)mm - (char *)m);
		mm->size = occupied;

		return &mm->mem;
	};

	if (freed.size() == 0) {
		if (used.size() == 0 || used.back()->top + occupied > mem_block_size) {
			mem_block * m = new mem_block;
			used.push(m);
			return new_block(m);
		}
		else {
			return simple_alloc(used.back());
		}
	}
	else {
		mem_block * m = freed.back();
		freed.pop();
		used.push(m);
		return new_block(m);
	}
}

void __free(void * ptr) {
	mem_header * mm = (mem_header *)((char *)ptr - (sizeof(int) + sizeof(unsigned int)));
	int offs = mm->start_offs;
	mm->start_offs = -1;
	mem_block * m = (mem_block *)((char *)mm - offs);
	m->available += mm->size;
	if (m->available == mem_block_size) {
		used.erase(find(used.begin(), used.end(), m));
		m->top = 0;
		freed.push(m);
	}
}

class string_atomizer {
	map<string, unsigned int> hash;
	vector<const string *> table;
public:
	string_atomizer() { }
	unsigned int get_atom(const string & s) {
		if (s.length() == 1)
			return s[0];
		else if (s.length() == 2)
			return (((unsigned int)s[1]) << 8) + s[0];
		else {
			map<string, unsigned int>::iterator it = hash.find(s);
			if (it != hash.end())
				return it->second;
			else {
				if (table.size() == table.capacity()) {
					table.reserve(table.size() + 2000);
				}
				hash[s] = 0;
				it = hash.find(s);
				table.push_back(&it->first);
				it->second = 65536 + table.size() - 1;
				return it->second;
			}
		}
	}
	const string get_string(unsigned int atom) {
		if (atom < 256) {
			char buf[2] = { (char) atom, 0 };
			return string(buf);
		}
		else if (atom < 65536) {
			char buf[3] = { (char)(atom & 0xFF), (char)(atom >> 8), 0 };
			return string(buf);
		}
		else {
			return *table[atom - 65536];
		}
	}
};

string_atomizer atomizer;

class interpreter {
private:
	predicate_item_user * starter;
public:
	map<string, predicate *> PREDICATES;
	map< string, vector<term *> *> DB;
	map< string, set<int> *> DBIndicators;
	map<string, value *> GVars;

	stack_container<predicate_item *> CALLS;
	stack_container<frame_item *> FRAMES;
	stack_container<bool> NEGS;
	stack_container<int> _FLAGS;
	stack_container<predicate_item_user *> PARENT_CALLS;
	stack_container<int> PARENT_CALL_VARIANT;
	stack_container<clause *> CLAUSES;
	std::list<int> FLAGS;
	stack_container<int> LEVELS;
	stack_container<generated_vars *> TRACE;
	stack_container<predicate_item *> TRACERS;
	stack_container<int> ptrTRACE;

	string current_output;
	string current_input;

	basic_istream<char> * ins;
	basic_ostream<char> * outs;

	int file_num;
	map<int, basic_fstream<char> > files;

	interpreter(const string & fname, const string & starter_name);
	~interpreter();

	string open_file(const string & fname, const string & mode);
	void close_file(const string & obj);
	basic_fstream<char> & get_file(const string & obj, int & fn);

	void block_process(bool clear_flag, bool cut_flag, predicate_item * frontier);

	double evaluate(frame_item * ff, const string & expression, int & p);

	bool check_consistency(set<string> & dynamic_prefixes);

	void add_std_body(string & body);

	vector<value *> * accept(frame_item * ff, predicate_item * current);
	vector<value *> * accept(frame_item * ff, clause * current);
	bool retrieve(frame_item * ff, clause * current, vector<value *> * vals, bool escape_vars);
	bool retrieve(frame_item * ff, predicate_item * current, vector<value *> * vals, bool escape_vars);
	bool process(bool neg, clause * this_clause_call, predicate_item * p, frame_item * f, vector<value *> * & positional_vals);

	void consult(const string & fname, bool renew);
	void bind();
	predicate_item_user * load_program(const string & fname, const string & starter_name);

	bool bypass_spaces(const string & s, int & p);
	value * parse(bool exit_on_error, bool parse_complex_terms, frame_item * ff, const string & s, int & p);
	void parse_program(vector<string> & renew, string & s);
	void parse_clause(vector<string> & renew, frame_item * ff, string & s, int & p);
	bool unify(frame_item * ff, value * from, value * to);
			
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

	virtual value * fill(frame_item * vars) = 0;
	virtual value * copy(frame_item * f) = 0;
	virtual bool unify(frame_item * ff, value * from) = 0;
	virtual bool defined() = 0;

	virtual string to_str(bool simple = false) = 0;

	virtual void escape_vars(frame_item * ff) = 0;

	virtual string make_str() {
		return to_str();
	}

	virtual void write(basic_ostream<char> * outs, bool simple = false) {
		(*outs) << to_str(simple);
	}

	void * operator new (size_t size){
		if (fast_memory_manager)
			return __alloc(size);
		else
			return ::operator new(size);
	}

	void operator delete (void * ptr) {
		if (fast_memory_manager)
			__free(ptr);
		else
			::operator delete(ptr);
	}
};

class frame_item {
private:
	vector<char> names;
	
	typedef struct {
		char * _name;
		value * ptr;
	} mapper;

	vector<mapper> vars;
public:
	frame_item() {
		names.reserve(32);
		vars.reserve(8);
	}

	~frame_item() {
		int it = 0;
		while (it < vars.size()) {
			vars[it].ptr->free();
			it++;
		}
	}

	void sync(frame_item * other) {
		int it = 0;
		while (it < other->vars.size()) {
			char * itc = other->vars[it]._name;
			set(itc, other->vars[it].ptr->copy(other));
			it++;
		}
	}

	frame_item * copy() {
		frame_item * result = new frame_item();
		result->names = names;
		long long d = &result->names[0] - &names[0];
		result->vars = vars;
		for (mapper & m : result->vars) {
			m.ptr = m.ptr->copy(this);
			m._name += d;
		}
		return result;
	}

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

	void set(const char * name, value * v) {
		bool found = false;
		int it = find(name, found);
		if (found) {
			vars[it].ptr->free();
			vars[it].ptr = v->copy(this);
		}
		else {
			char * oldp = &names[0];
			int oldn = names.size();
			int n = strlen(name);
			names.resize(oldn + n + 1);
			char * newp = &names[0];
			for (int i = 0; i < vars.size(); i++)
				vars[i]._name += newp - oldp;
			char * _name = newp + oldn;
			strcpy(_name, name);
			mapper m = { _name, v->copy(this) };
			vars.insert(vars.begin() + it, m);
		}
	}

	value * get(const char * name) {
		bool found = false;
		int it = find(name, found);
		return found ? vars[it].ptr : NULL;
	}
};

class any : public value {
public:
	any() : value() { }

	virtual void escape_vars(frame_item * ff) { }

	virtual value * fill(frame_item * vars) { return this; }
	virtual value * copy(frame_item * f) { return new any(); }
	virtual bool unify(frame_item * ff, value * from) {
		return true;
	}
	virtual bool defined() {
		return false;
	};

	virtual string to_str(bool simple = false) { return "_"; }
};

class var : public value {
	string name;
public:
	var(const string & _name) : value(), name(_name) { }

	virtual void escape_vars(frame_item * ff) {
		value * v = ff->get(name.c_str());
		if (v) {
			v->escape_vars(ff);
		}
		ff->escape(name.c_str(), '$');
		name = "$" + name;
	}

	virtual value * fill(frame_item * vars) {
		value * v = vars->get(name.c_str());
		if (v)
			return v->copy(vars);
		else {
			return this;
		}
	}

	virtual value * copy(frame_item * f) {
		value * v = f->get(name.c_str());
		if (v)
			return v->copy(f);
		else
			return new var(name);
	}

	virtual void write(basic_ostream<char> * outs, bool simple = false) {
		(*outs) << name;
	}

	virtual const string & get_name() { return name; }

	virtual bool unify(frame_item * ff, value * from) {
		if (dynamic_cast<var *>(from) || dynamic_cast<any *>(from))
			return true;
		value * v = ff->get(name.c_str());
		if (v)
			return v->unify(ff, from);
		else
			ff->set(name.c_str(), from);
		return true;
	}
	
	virtual bool defined() {
		return false;
	};

	virtual string to_str(bool simple = false) { return "__VAR__" + name; }
};

class int_number : public value {
	friend class float_number;
private:
	long long v;
public:
	int_number(long long _v) : value(), v(_v) { }

	virtual void escape_vars(frame_item * ff) {	}

	virtual value * fill(frame_item * vars) { return this; }
	virtual value * copy(frame_item * f) { return new int_number(v); }
	virtual bool unify(frame_item * ff, value * from);
	virtual bool defined() {
		return true;
	};

	virtual double get_value() { use(); return v; }

	virtual string to_str(bool simple = false) {
		char buf[65];

		return string(__ltoa(v, buf, 10));
	}

	void inc() { v++; }
	void dec() { v--; }
};

class float_number : public value {
	friend class int_number;
private:
	double v;
public:
	float_number(double _v) : value(), v(_v) { }

	virtual void escape_vars(frame_item * ff) {	}

	virtual value * fill(frame_item * vars) { return this; }
	virtual value * copy(frame_item * f) { return new float_number(v); }
	virtual bool unify(frame_item * ff, value * from) {
		if (dynamic_cast<any *>(from)) return true;
		if (dynamic_cast<var *>(from)) { ff->set(((var *)from)->get_name().c_str(), this); return true; }
		if (dynamic_cast<float_number *>(from))
			return v == ((float_number *)from)->v;
		else if (dynamic_cast<int_number *>(from))
			return v == ((int_number *)from)->v;
		else
			return false;
	}
	virtual bool defined() {
		return true;
	};

	virtual double get_value() { use();  return v; }

	virtual string to_str(bool simple = false) {
		char buf[65];

		sprintf(buf, "%lf", v);
		return string(buf);
	}

	void inc() { v++; }
	void dec() { v--; }
};

bool int_number::unify(frame_item * ff, value * from) {
	if (dynamic_cast<any *>(from)) return true;
	if (dynamic_cast<var *>(from)) { ff->set(((var *)from)->get_name().c_str(), this); return true; }
	if (dynamic_cast<int_number *>(from))
		return v == ((int_number *)from)->v;
	else if (dynamic_cast<float_number *>(from))
		return v == ((float_number *)from)->v;
	else
		return false;
}

class term : public value {
private:
	unsigned int name;
	vector<value *> args;
public:
	term(const string & _name) : value() {
		name = atomizer.get_atom(_name);
	}

	term(const term & src) {
		this->refs = 1;
		this->name = src.name;
		this->args = vector<value *>();
	}

	virtual void free() {
		for (int i = 0; i < args.size(); i++)
			args[i]->free();
		value::free();
	}

	virtual void use() {
		value::use();
		for (int i = 0; i < args.size(); i++)
			args[i]->use();
	}

	virtual void escape_vars(frame_item * ff) {
		for (int i = 0; i < args.size(); i++)
			args[i]->escape_vars(ff);
	}

	virtual const string get_name() { return atomizer.get_string(name); }

	virtual const vector<value *> & get_args() { return args; }

	virtual value * fill(frame_item * vars) {
		for (int i = 0; i < args.size(); i++) {
			value * old = args[i];
			args[i] = args[i]->fill(vars);
			if (args[i] != old) old->free();
		}
		return this;
	}

	virtual void add_arg(frame_item * f, value * v) {
		args.push_back(v->copy(f));
	}

	virtual value * copy(frame_item * f) {
		term * result = new term(*this);
		for (int i = 0; i < args.size(); i++)
			result->add_arg(f, args[i]);
		return result;
	}
	virtual bool unify(frame_item * ff, value * from) {
		if (dynamic_cast<any *>(from)) return true;
		if (dynamic_cast<var *>(from)) { ff->set(((var *)from)->get_name().c_str(), this); return true; }
		if (dynamic_cast<term *>(from)) {
			term * v2 = ((term *)from);
			if (name != v2->name || args.size() != v2->args.size())
				return false;
			for (int i = 0; i < args.size(); i++)
				if (!args[i]->unify(ff, v2->args[i]))
					return false;
			return true;
		}
		else
			return false;
	}
	virtual bool defined() {
		for (int i = 0; i < args.size(); i++)
			if (!args[i]->defined())
				return false;
		return true;
	};

	virtual string to_str(bool simple = false) {
		string result = atomizer.get_string(name);
		if (args.size() > 0) {
			result += "(";
			for (int i = 0; i < args.size() - 1; i++) {
				result += args[i]->to_str(simple);
				result += ",";
			}
			result += args[args.size() - 1]->to_str(simple);
			result += ")";
		}
		return result;
	}
};

class indicator : public value {
private:
	string name;
	int arity;
public:
	indicator(const string & _name, int _arity) : value(), name(_name), arity(_arity) { }

	virtual void escape_vars(frame_item * ff) {	}

	virtual const string & get_name() { return name; }

	virtual int get_arity() { return arity; }

	virtual value * fill(frame_item * vars) {
		return this;
	}

	virtual value * copy(frame_item * f) {
		return new indicator(name, arity);
	}
	virtual bool unify(frame_item * ff, value * from) {
		if (dynamic_cast<any *>(from)) return true;
		if (dynamic_cast<var *>(from)) { ff->set(((var *)from)->get_name().c_str(), this); return true; }
		if (dynamic_cast<indicator *>(from)) {
			indicator * v2 = ((indicator *)from);
			return name == v2->name && arity == v2->arity;
		}
		else
			return false;
	}
	virtual bool defined() {
		return true;
	};

	virtual string to_str(bool simple = false) {
		string result = name;

		char buf[65];
		result += "/";
		result += __itoa(arity, buf, 10);
		
		return result;
	}
};

class list : public value {
	friend class predicate_item_nth;

	std::list<value *> val;
	value * tag;
public:
	list(const std::list<value *> & v, value * _tag) : value(), val(v), tag(_tag) { }

	virtual void free() {
		for (value * v : val)
			v->free();
		if (tag) tag->free();
		value::free();
	}

	virtual void use() {
		value::use();
		for (value * v : val)
			v->use();
		if (tag) tag->use();
	}

	virtual void escape_vars(frame_item * ff) {
		for (value * v : val)
			v->escape_vars(ff);
		if (tag)
			tag->escape_vars(ff);
	}

	int size() {
		int result = val.size();
		if (tag)
			if (dynamic_cast<list *>(tag))
				result += ((list *)tag)->size();
			else
				result++;
		return result;
	}

	value * get_last() {
		if (tag)
			if (dynamic_cast<list *>(tag))
				return ((list *)tag)->get_last();
			else {
				tag->use();
				return tag;
			}
		else if (val.size() == 0)
			return NULL;
		else {
			std::list<value *>::iterator it = val.end();

			(*(--it))->use();

			return *it;
		}
	}

	value * get_nth(int n) {
		if (n < 1) return NULL;

		if (n <= val.size()) {
			std::list<value *>::iterator it = val.begin();
			while (n > 1) {
				n--;
				it++;
			}
			(*it)->use();
			return *it;
		}
		else
			if (tag)
				if (dynamic_cast<list *>(tag))
					return ((list *)tag)->get_nth(n - val.size());
				else {
					tag->use();
					return tag;
				}
			else
				return NULL;
	}

	void iterate(std::function<void(value *)> check) {
		std::list<value *>::iterator it = val.begin();
		while (it != val.end()) {
			check(*it);
			it++;
		}
		if (tag)
			if (dynamic_cast<list *>(tag))
				((list *)tag)->iterate(check);
			else
				check(tag);
	}

	void split(frame_item * f, int p, value * & L1, value * & L2) {
		std::list<value *> S, S1, S2;
		get(f, S);

		std::list<value *>::iterator it = S.begin();
		for (int i = 0; i < p; i++)
			S1.push_back((*it++)->copy(f));
		for (int i = p; i < S.size(); i++)
			S2.push_back((*it++)->copy(f));
		L1 = new list(S1, NULL);
		L2 = new list(S2, NULL);
	}

	list * from(frame_item * f, std::list<value *>::iterator starting) {
		list * result = new list(std::list<value *>(), NULL);
		while (starting != val.end())
		{
			result->add((*starting)->copy(f));
			starting++;
		}
		if (tag) result->set_tag(tag->copy(f));
		return result;
	}

	virtual value * fill(frame_item * vars) {
		std::list<value *>::iterator it = val.begin();
		for (; it != val.end(); it++) {
			value * old = *it;
			*it = (*it)->fill(vars);
			if (*it != old) old->free();
		}
		if (tag) {
			value * old = tag;
			tag = tag->fill(vars);
			if (old && old != tag) old->free();
		}
		return this;
	}

	virtual list * append(frame_item * f, list * L2) {
		list * result = ((list *)copy(f));
		for (value * v : L2->val) {
			result->add(v->copy(f));
		}
		if (L2->tag)
			if (dynamic_cast<list *>(L2->tag))
				return result->append(f, ((list*)L2->tag));
			else
				result->add(L2->tag->copy(f));
		return result;
	}

	virtual value * copy(frame_item * f) {
		std::list<value *> new_val;
		for (value * v : val)
			new_val.push_back(v->copy(f));
		return new list(new_val, tag ? tag->copy(f) : NULL);
	}

	virtual void add(value * v) {
		if (tag)
			if (dynamic_cast<list *>(tag))
				((list *)tag)->add(v);
			else {
				std::cout << "Adding to non-list tag?!" << endl;
				exit(1);
			}
		else {
			val.push_back(v);
		}
	}

	virtual void set_tag(value * new_tag) {
		if (tag) tag->free();
		if (dynamic_cast<list *>(new_tag) && ((list *)new_tag)->size() == 0)
			new_tag = NULL;
		tag = new_tag;
		if (new_tag) new_tag->use();
	}

	virtual bool get(frame_item * f, std::list<value *> & dest) {
		dest.clear();
		for (value * v : val)
			dest.push_back(v->copy(f));
		if (tag) {
			if (dynamic_cast<list *>(tag)) {
				std::list<value *> ltag;
				if (((list *)tag)->get(f, ltag)) {
					for (value * v : ltag)
						dest.push_back(v->copy(f));
					return true;
				}
				else
					return false;
			}
			dest.push_back(tag->copy(f));
			return true;
		}
		else return true;
	}

	virtual bool unify(frame_item * ff, value * from) {
		if (dynamic_cast<any *>(from)) return true;
		if (dynamic_cast<var *>(from)) {
			if (((var *)from)->defined()) ff->set(((var *)from)->get_name().c_str(), this);
			return true;
		}
		if (dynamic_cast<list *>(from)) {
			value * _from = (list *)from;
			std::list<value *>::iterator _from_it = ((list *)_from)->val.begin();
			value * _to = this;
			std::list<value *>::iterator _to_it = ((list *)_to)->val.begin();

			while (_from && _to) {
				while (dynamic_cast<list *>(_from) && _from_it == ((list *)_from)->val.end()) {
					_from = ((list *)_from)->tag;
					if (dynamic_cast<list *>(_from)) _from_it = ((list *)_from)->val.begin();
				}
				while (dynamic_cast<list *>(_to) && _to_it == ((list *)_to)->val.end()) {
					_to = ((list *)_to)->tag;
					if (dynamic_cast<list *>(_to)) _to_it = ((list *)_to)->val.begin();
				}
				if (dynamic_cast<any *>(_from) || dynamic_cast<any *>(_to))
					return true;
				if (dynamic_cast<var *>(_from))
					if (dynamic_cast<list *>(_to))
						return _from->unify(ff, ((list *)_to)->from(ff, _to_it));
					else if (_to)
						return _from->unify(ff, _to);
					else
						return _from->unify(ff, new ::list(std::list<value *>(), NULL));
				if (dynamic_cast<var *>(_to))
					if (dynamic_cast<list *>(_from))
						return _to->unify(ff, ((list *)_from)->from(ff, _from_it));
					else if (_from)
						return _to->unify(ff, _from);
					else
						return _to->unify(ff, new ::list(std::list<value *>(), NULL));
				if (dynamic_cast<list *>(_from) && dynamic_cast<list *>(_to))
					if (!(*_to_it++)->unify(ff, *_from_it++))
						return false;
			}

			return !_from && !_to;
		} else
			return false;
	}
	virtual bool defined() {
		for (value * v : val)
			if (!v->defined())
				return false;
		if (tag)
			return tag->defined();
		return true;
	};

	virtual string make_str() {
		string result;
		for (value * v : val)
			result += v->make_str();
		if (tag)
			result += tag->make_str();
		return result;
	}

	virtual string to_str(bool simple = false) {
		string result;

		int k = 0;

		if (!simple) result += "[";
		for (value * v : val) {
			result += v->to_str(false);
			k++;
			if (k < val.size() || tag && !(dynamic_cast<list *>(tag) && dynamic_cast<list *>(tag)->size() == 0))
				result += ",";
		}
		if (tag) {
			result += tag->to_str(true);
		}
		if (!simple) result += "]";

		return result;
	}
};

double interpreter::evaluate(frame_item * ff, const string & expression, int & p) {
	string ops = "(+-*/";
	int priors[5] = { 0, 1, 1, 2, 2 };

	stack<char> opers;

	typedef struct {
		bool is_op;
		union {
			char op;
			double v;
		};
	} pitem;

	queue<pitem> postfix;
	stack<double> vals;
	bool can_be_neg = true;

	int bracket_level = 0;

	auto calc_one = [&]() {
		double v2 = vals.top();
		vals.pop();
		double v1 = vals.top();
		vals.pop();

		double v;

		switch (postfix.front().op) {
			case '+': v = v1 + v2; break;
			case '-': v = v1 - v2; break;
			case '*': v = v1 * v2; break;
			case '/': v = v1 / v2; break;
		}
		vals.push(v);
		postfix.pop();
	};

	auto get_arg = [&](int & p, char ending)->double {
		double result = evaluate(ff, expression, p);
		if (p >= expression.length() || expression[p] != ending) {
			std::cout << "Expression '" << expression << "' : '" << ending << "' expected!" << endl;
			exit(105);
		}
		p++;
		bypass_spaces(expression, p);
		return result;
	};

	bypass_spaces(expression, p);
	while (p < expression.length() && expression[p] != ',') {
		if (expression[p] == '(') {
			bracket_level++;
			opers.push(expression[p]);
			can_be_neg = true;
			p++;
		}
		else if (expression[p] == ')') {
			if (bracket_level == 0) break;
			bracket_level--;
			p++;
			while (opers.size()) {
				if (opers.top() == '(') break;
				pitem p = { true };
				p.op = opers.top();
				postfix.push(p);
				opers.pop();
			}
			if (opers.size() == 0) {
				std::cout << "Evaluate(" << expression << ") : brackets disbalance!" << endl;
				exit(105);
			}
			opers.pop();
			can_be_neg = false;
		}
		else {
			string::size_type pos = ops.find(expression[p]);
			if (pos == string::npos || expression[p] == '-' && can_be_neg) {
				double sign = +1.0;
				if (expression[p] == '-') {
					sign = -1.0;
					p++;
				}
				value * v = parse(false, false, ff, expression, p);
				if (dynamic_cast<int_number *>(v)) {
					pitem p = { false };
					p.v = sign*dynamic_cast<int_number *>(v)->get_value();
					postfix.push(p);
				}
				else if (dynamic_cast<float_number *>(v)) {
					pitem p = { false };
					p.v = sign*dynamic_cast<float_number *>(v)->get_value();
					postfix.push(p);
				}
				else if (dynamic_cast<var *>(v) || dynamic_cast<term *>(v)) {
					string id = dynamic_cast<var *>(v) ? ((var*)v)->get_name() : ((term *)v)->get_name();
					pitem pp = { false };
					bypass_spaces(expression, p);
					if (p < expression.length() && expression[p] == '(') {
						p++;
						if (id == "max") {
							double a = get_arg(p, ',');
							double b = get_arg(p, ')');
							pp.v = sign*max(a, b);
							postfix.push(pp);
						}
						else if (id == "abs") {
							double a = get_arg(p, ')');
							pp.v = sign*fabs(a);
							postfix.push(pp);
						}
						else if (id == "round") {
							double a = get_arg(p, ')');
							pp.v = sign*round(a);
							postfix.push(pp);
						}
						else {
							std::cout << "Evaluation(" << expression << ") : unknown function '" << id << "'!" << endl;
							exit(105);
						}
					}
					else {
						std::cout << "Evaluation(" << expression << ") : Var(" << id << ") not defined!" << endl;
						exit(105);
					}
				} else {
					std::cout << "Evaluate error : " << expression << endl;
					exit(105);
				}
				v->free();
				can_be_neg = false;
			}
			else {
				while (opers.size()) {
					if (priors[pos] > priors[ops.find(opers.top())]) break;
					pitem p = { true };
					p.op = opers.top();
					postfix.push(p);
					opers.pop();
				}
				opers.push(expression[p]);
				p++;
				can_be_neg = true;
			}
		}
		bypass_spaces(expression, p);
	}
	while (opers.size()) {
		pitem p = { true };
		p.op = opers.top();
		postfix.push(p);
		opers.pop();
	}

	while (postfix.size()) {
		pitem p = postfix.front();
		if (p.is_op)
			calc_one();
		else {
			vals.push(p.v);
			postfix.pop();
		}
	}

	if (vals.size() != 1) {
		std::cout << "Strange evaluation(" << expression << ")!" << endl;
		exit(105);
	}

	return vals.top();
}

class predicate {
	friend predicate_item;
	friend clause;
private:
	string name;
	vector<clause *> clauses;
public:
	predicate(const string & _name) : name(_name) { }
	~predicate();

	void bind();

	void add_clause(clause * c) {
		clauses.push_back(c);
	}

	virtual int num_clauses() { return clauses.size(); }
	virtual clause * get_clause(int i) { return clauses[i]; }
};

class clause {
	friend predicate_item;
private:
	predicate * parent;
	std::list<string> args;
	std::list<value *> _args;
	vector<predicate_item *> items;
public:
	clause(predicate * pp) : parent(pp) { }
	~clause();

	virtual void bind();

	predicate * get_predicate() { return parent; }

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

void predicate::bind() {
	for (clause * c : clauses)
		c->bind();
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

	virtual frame_item * get_next_variant(int i) { return at(i); }

	virtual void delete_from(int i) {
		for (int j = i; j < size(); j++)
			delete at(j);
	}
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
public:
	predicate_item(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : neg(_neg), once(_once), call(_call), self_number(num), parent(c), prolog(_prolog) { }
	~predicate_item() {
		for (value * v : _args)
			v->free();
	}

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
		} else
			return NULL;
	}

	virtual frame_item * get_next_variant(frame_item * f, int & internal_variant, vector<value *> * positional_vals) { return NULL; }

	const std::list<string> * get_args() {
		return &args;
	}

	clause * get_parent() { return parent; }

	bool is_first() { return self_number == 0; }
	bool is_last() { return !parent || self_number == parent->items.size() - 1; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) = 0;

	virtual bool execute(frame_item * f) {
		return true;
	}
};

class lazy_generated_vars : public generated_vars {
	predicate_item * p;
	int internal_variant;
	unsigned int max_num_variants;

	vector<value *> * positional_vals;

	frame_item * parent;
public:
	lazy_generated_vars(frame_item * f, predicate_item * pp, vector<value *> * _positional_vals, frame_item * first, int internal_ptr, unsigned int max_nv) : generated_vars() {
		p = pp;
		internal_variant = internal_ptr;
		max_num_variants = max_nv;

		positional_vals = _positional_vals;

		parent = f;

		if (first)
			this->push_back(first);
	}

	virtual bool has_variant(int i) { return i < this->size(); }

	virtual void trunc_from(int k) {
		if (k < max_num_variants) max_num_variants = k;
		delete_from(k);
	}

	virtual frame_item * get_next_variant(int i) {
		frame_item * result = this->at(i);

		if (this->size() < max_num_variants) {
			frame_item * next = p->get_next_variant(parent, internal_variant, positional_vals);
			if (next)
				this->push_back(next);
		}

		return result;
	}
	
	virtual void delete_from(int k) {
		for (int i = k; i < size(); i++) {
			delete this->at(i);
		}
		this->resize(k);
	}
};

void clause::bind() {
	for (predicate_item * p : items)
		p->bind();
}

predicate::~predicate() {
	for (clause * c : clauses) {
		delete c;
	}
}

clause::~clause() {
	for (predicate_item * p : items)
		delete p;
	for (value * v : _args)
		v->free();
}

class predicate_or : public predicate_item {
protected:
	predicate_item * ending;
public:
	predicate_or(int num, clause * c, interpreter * _prolog) : predicate_item(false, false, false, num, c, _prolog) {
		ending = NULL;
	}

	virtual const string get_id() { return "or"; }

	void set_ending(predicate_item * p) {
		ending = p;
	}

	virtual predicate_item * get_next(int variant) {
			return ending;
	}

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);
		return result;
	}
};

class predicate_left_bracket : public predicate_item {
protected:
	vector<predicate_item *> branches;
	bool inserted;
public:
	predicate_left_bracket(bool _inserted, bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog), inserted(_inserted) { }

	bool is_inserted() { return inserted; }

	virtual const string get_id() { return "left_bracket"; }

	void push_branch(predicate_item * p) {
		branches.push_back(p);
	}

	virtual predicate_item * get_next(int variant) {
		if (variant == 0)
			return predicate_item::get_next(variant);
		return branches[variant-1];
	}

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		for (int i = 0; i <= branches.size(); i++) {
			frame_item * r = f->copy();
			result->push_back(r);
		}
		return result;
	}
};

class predicate_right_bracket : public predicate_item {
	predicate_left_bracket * left_bracket;
public:
	predicate_right_bracket(predicate_left_bracket * lb, int num, clause * c, interpreter * _prolog) :
		predicate_item(false, false, false, num, c, _prolog),
		left_bracket(lb) { }

	virtual const string get_id() { return "right_bracket"; }

	predicate_left_bracket * get_corresponding() { return left_bracket; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);
		return result;
	}
};

class predicate_item_eq : public predicate_item {
public:
	predicate_item_eq(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "eq"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "eq(A,B) incorrect call!" << endl;
			exit(-3);
		}
		var * a1 = dynamic_cast<var *>(positional_vals->at(0));
		var * a2 = dynamic_cast<var *>(positional_vals->at(1));
		if (a1 && a2) {
			std::cout << "eq(A,B) unbounded!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);
		if (!a1 && !a2)
		{
			if (!positional_vals->at(0)->unify(r, positional_vals->at(1))) {
				delete r;
				delete result;
				result = NULL;
			}
		}
		else if (a1 && !a2) {
			r->set(a1->get_name().c_str(), positional_vals->at(1));
		}
		else if (!a1 && a2) {
			r->set(a2->get_name().c_str(), positional_vals->at(0));
		}
		return result;
	}
};

class predicate_item_neq : public predicate_item {
public:
	predicate_item_neq(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "neq"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "neq(A,B) incorrect call!" << endl;
			exit(-3);
		}
		var * a1 = dynamic_cast<var *>(positional_vals->at(0));
		var * a2 = dynamic_cast<var *>(positional_vals->at(1));
		if (a1 || a2) {
			std::cout << "neq(A,B) unbounded!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);
		if (positional_vals->at(0)->unify(r, positional_vals->at(1))) {
			delete r;
			delete result;
			result = NULL;
		}
		return result;
	}
};

class predicate_item_less : public predicate_item {
public:
	predicate_item_less(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "less"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "less(A,B) incorrect call!" << endl;
			exit(-3);
		}

		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1 || !d2) {
			std::cout << "less(A,B) unbounded!" << endl;
			exit(-3);
		}

		int_number * i1 = dynamic_cast<int_number *>(positional_vals->at(0));
		float_number * f1 = dynamic_cast<float_number *>(positional_vals->at(0));
		term * t1 = dynamic_cast<term *>(positional_vals->at(0));
		int_number * i2 = dynamic_cast<int_number *>(positional_vals->at(1));
		float_number * f2 = dynamic_cast<float_number *>(positional_vals->at(1));
		term * t2 = dynamic_cast<term *>(positional_vals->at(1));

		generated_vars * result = new generated_vars();
		if (i1 && i2 && i1->get_value() < i2->get_value() ||
			f1 && f2 && f1->get_value() < f2->get_value() ||
			i1 && f2 && i1->get_value() < f2->get_value() ||
			f1 && i2 && f1->get_value() < i2->get_value() ||
			t1 && t2 && t1->get_args().size() == 0 && t2->get_args().size() == 0 && t1->get_name() < t2->get_name()) {
			frame_item * r = f->copy();
			result->push_back(r);
		}
		else {
			delete result;
			result = NULL;
		}
		return result;
	}
};

class predicate_item_greater : public predicate_item {
public:
	predicate_item_greater(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "greater"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "greater(A,B) incorrect call!" << endl;
			exit(-3);
		}

		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1 || !d2) {
			std::cout << "greater(A,B) unbounded!" << endl;
			exit(-3);
		}

		int_number * i1 = dynamic_cast<int_number *>(positional_vals->at(0));
		float_number * f1 = dynamic_cast<float_number *>(positional_vals->at(0));
		term * t1 = dynamic_cast<term *>(positional_vals->at(0));
		int_number * i2 = dynamic_cast<int_number *>(positional_vals->at(1));
		float_number * f2 = dynamic_cast<float_number *>(positional_vals->at(1));
		term * t2 = dynamic_cast<term *>(positional_vals->at(1));

		generated_vars * result = new generated_vars();
		if (i1 && i2 && i1->get_value() > i2->get_value() ||
			f1 && f2 && f1->get_value() > f2->get_value() ||
			i1 && f2 && i1->get_value() > f2->get_value() ||
			f1 && i2 && f1->get_value() > i2->get_value() ||
			t1 && t2 && t1->get_args().size() == 0 && t2->get_args().size() == 0 && t1->get_name() > t2->get_name()) {
			frame_item * r = f->copy();
			result->push_back(r);
		}
		else {
			delete result;
			result = NULL;
		}
		return result;
	}
};

class predicate_item_inc : public predicate_item {
public:
	predicate_item_inc(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "inc"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "inc(A,B) incorrect call!" << endl;
			exit(-3);
		}
		var * a1 = dynamic_cast<var *>(positional_vals->at(0));
		var * a2 = dynamic_cast<var *>(positional_vals->at(1));
		if (a1 && a2) {
			std::cout << "inc(A,B) unbounded!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);
		if (!a1 && !a2)
		{
			int_number * n1 = dynamic_cast<int_number *>(positional_vals->at(0));
			int_number * n2 = dynamic_cast<int_number *>(positional_vals->at(1));
			n1->inc();

			if (!n1 || !n2 || !n1->unify(r, n2)) {
				delete r;
				delete result;
				result = NULL;
			}
		}
		else if (a1 && !a2) {
			int_number * n2 = dynamic_cast<int_number *>(positional_vals->at(1));
			n2->dec();

			r->set(a1->get_name().c_str(), n2);
			n2->free();
		}
		else if (!a1 && a2) {
			int_number * n1 = dynamic_cast<int_number *>(positional_vals->at(0));
			n1->inc();

			r->set(a2->get_name().c_str(), n1);
			n1->free();
		}
		return result;
	}
};

class predicate_item_term_split : public predicate_item {
public:
	predicate_item_term_split(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "split(=..)"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "=..(term,[term_id,arg1,...,argN]) incorrect call!" << endl;
			exit(-3);
		}

		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();

		if (!d1 && !d2) {
			std::cout << "=..(term,[term_id,arg1,...,argN]) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();

		if (d1) {
			term * t1 = dynamic_cast<term *>(positional_vals->at(0));
			value * t2 = dynamic_cast<value *>(positional_vals->at(1));

			if (!t1) {
				std::cout << "=..(term,[term_id,arg1,...,argN]) has incorrect parameters!" << endl;
				exit(-3);
			}

			frame_item * r = f->copy();

			std::list<value *> LL;
			LL.push_back(new term(t1->get_name()));

			for (int i = 0; i < t1->get_args().size(); i++)
				LL.push_back(t1->get_args().at(i)->copy(r));

			::list * L2 = new ::list(LL, NULL);

			if (t2->unify(r, L2)) {
				result->push_back(r);
			}
			else {
				delete result;
				result = NULL;
				delete r;
			}
			L2->free();
		}
		else if (d2) {
			value * t1 = dynamic_cast<value *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));

			if (!L2 || L2->size() == 0 || !dynamic_cast<term *>(L2->get_nth(1))) {
				std::cout << "=..(term,[term_id,arg1,...,argN]) has incorrect parameters!" << endl;
				exit(-3);
			}

			frame_item * r = f->copy();

			term * TT = new term(dynamic_cast<term *>(L2->get_nth(1))->get_name());
			for (int i = 2; i <= L2->size(); i++)
				TT->add_arg(r, L2->get_nth(i)->copy(r));

			if (t1->unify(r, TT)) {
				result->push_back(r);
			}
			else {
				delete result;
				result = NULL;
				delete r;
			}
			TT->free();
		}

		return result;
	}
};

class predicate_item_g_assign : public predicate_item {
public:
	predicate_item_g_assign(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "g_assign"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "g_assign(Name,Val) incorrect call!" << endl;
			exit(-3);
		}
		term * a1 = dynamic_cast<term *>(positional_vals->at(0));
		value * a2 = dynamic_cast<value *>(positional_vals->at(1));
		if (!a1 || !a1->defined() || a1->get_args().size() > 0 || !a2 || !a2->defined()) {
			std::cout << "g_assign(Name,Val) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);

		prolog->GVars[a1->get_name()] = a2->copy(r);

		return result;
	}
};

class predicate_item_g_read : public predicate_item {
public:
	predicate_item_g_read(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "g_read"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "g_read(Name,A) incorrect call!" << endl;
			exit(-3);
		}
		term * a1 = dynamic_cast<term *>(positional_vals->at(0));
		value * a2 = dynamic_cast<value *>(positional_vals->at(1));
		if (!a1 || !a1->defined() || a1->get_args().size() > 0 || !a2) {
			std::cout << "g_read(Name,A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);

		map<string, value *>::iterator it = prolog->GVars.find(a1->get_name());
		if (it == prolog->GVars.end()) {
			value * v = new int_number(0);
			if (!a2->unify(r, v)) {
				delete result;
				result = NULL;
				delete r;
			}
			v->free();
		}
		else {
			if (!a2->unify(r, it->second)) {
				delete result;
				result = NULL;
				delete r;
			}
		}

		return result;
	}
};

class predicate_item_cut : public predicate_item {
	predicate_item * frontier;
public:
	predicate_item_cut(predicate_item * fr, int num, clause * c, interpreter * _prolog) :
		predicate_item(false, false, false, num, c, _prolog),
		frontier(fr)
	{ }

	virtual const string get_id() { return "cut(!)"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		prolog->block_process(false, true, frontier);
		generated_vars * result = new generated_vars();
		result->push_back(f->copy());
		return result;
	}
};

class predicate_item_is : public predicate_item {
	string var_name;
	string expression;
public:
	predicate_item_is(const string & _var_name, const string & _expr, bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog),
		var_name(_var_name), expression(_expr)
	{ }

	virtual const string get_id() { return "is"; }

	virtual const string & get_expression() { return expression; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		
		// Evaluate expression
		int p = 0;
		double _res = prolog->evaluate(f, expression, p);

		if (p < expression.length()) {
			std::cout << "'is' evaluation : can't understand the following : '" << expression.substr(p) << "'" << endl;
			exit(2000);
		}

		value * res;
		if (_res == (long long)_res)
			res = new int_number((long long)_res);
		else
			res = new float_number(_res);

		frame_item * ff = f->copy();
		value * v = f->get(var_name.c_str());
		if (!v || !v->defined()) {
			ff->set(var_name.c_str(), res);
			result->push_back(ff);
		} else if (v->unify(ff, res))
			result->push_back(ff);
		else {
			delete ff;
			delete result;
			result = NULL;
		}

		res->free();

		return result;
	}
};

class predicate_item_fail : public predicate_item {
public:
	predicate_item_fail(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "fail"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		return NULL;
	}
};

class predicate_item_true : public predicate_item {
public:
	predicate_item_true(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "true"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		result->push_back(f->copy());
		return result;
	}
};

class predicate_item_append : public predicate_item {
	bool d1, d2, d3;
public:
	predicate_item_append(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "append"; }

	virtual frame_item * get_next_variant(frame_item * f, int & internal_variant, vector<value *> * positional_vals) {
		frame_item * result = NULL;

		if (d1 && d2 && d3 && internal_variant == 0) {
			::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));
			::list * L3 = dynamic_cast<::list *>(positional_vals->at(2));

			if (L1 && L2 && L3) {
				::list * conc = L1->append(f, L2);
				frame_item * r = f->copy();
				if (conc->unify(r, L3))
					result = r;
				else {
					delete r;
				}
				conc->free();
			}
			internal_variant++;
		}
		if (d1 && d2 && !d3 && internal_variant == 0) {
			::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));
			value * L3 = dynamic_cast<::value *>(positional_vals->at(2));

			if (L1 && L2 && L3) {
				::list * conc = L1->append(f, L2);
				frame_item * r = f->copy();
				if (L3->unify(r, conc))
					result = r;
				else {
					delete r;
				}
				conc->free();
			}
			internal_variant++;
		}
		if (d1 && !d2 && d3) {
			::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));
			value * L2 = dynamic_cast<::value *>(positional_vals->at(1));
			::list * L3 = dynamic_cast<::list *>(positional_vals->at(2));

			if (L1 && L2 && L3)
				for (; !result && internal_variant <= L3->size(); internal_variant++) {
					value * K1 = NULL;
					value * K2 = NULL;
					L3->split(f, internal_variant, K1, K2);

					frame_item * r = f->copy();

					if (L1->unify(r, K1) && L2->unify(r, K2))
						result = r;
					else {
						delete r;
					}
					K1->free();
					K2->free();
				}
		}
		if (!d1 && d2 && d3) {
			value * L1 = dynamic_cast<::value *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));
			::list * L3 = dynamic_cast<::list *>(positional_vals->at(2));

			if (L1 && L2 && L3)
				for (; !result && internal_variant <= L3->size(); internal_variant++) {
					value * K1;
					value * K2;
					L3->split(f, internal_variant, K1, K2);

					frame_item * r = f->copy();

					if (L1->unify(r, K1) && L2->unify(r, K2))
						result = r;
					else {
						delete r;
					}
					K1->free();
					K2->free();
				}
		}
		if (!d1 && !d2 && d3 || !d1 && !d2 && !d3) {
			value * L1 = dynamic_cast<::value *>(positional_vals->at(0));
			value * L2 = dynamic_cast<::value *>(positional_vals->at(1));
			::list * L3 = dynamic_cast<::list *>(positional_vals->at(2));

			if (L1 && L2 && L3)
				for (; !result && internal_variant <= L3->size(); internal_variant++) {
					value * K1 = new ::list(std::list<value*>(), NULL);
					value * K2 = new ::list(std::list<value*>(), NULL);
					L3->split(f, internal_variant, K1, K2);

					frame_item * r = f->copy();

					if (L1->unify(r, K1) && L2->unify(r, K2))
						result = r;
					else {
						delete r;
					}
					K1->free();
					K2->free();
				}
		}

		return result;
	}

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 3) {
			std::cout << "append(A,B,C) incorrect call!" << endl;
			exit(-3);
		}
		d1 = positional_vals->at(0)->defined();
		d2 = positional_vals->at(1)->defined();
		d3 = positional_vals->at(2)->defined();

		int internal_ptr = 0;
		frame_item * first = get_next_variant(f, internal_ptr, positional_vals);
		
		if (first)
			return new lazy_generated_vars(f, this, positional_vals, first, internal_ptr, once ? 1 : 0xFFFFFFFF);
		else
			return NULL;
	}
};

class predicate_item_atom_concat : public predicate_item {
	bool d1, d2, d3;
public:
	predicate_item_atom_concat(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "atom_concat"; }

	virtual frame_item * get_next_variant(frame_item * f, int & internal_variant, vector<value *> * positional_vals) {
		frame_item * result = NULL;

		if (d1 && d2 && d3 && internal_variant == 0) {
			::term * L1 = dynamic_cast<::term *>(positional_vals->at(0));
			::term * L2 = dynamic_cast<::term *>(positional_vals->at(1));
			::term * L3 = dynamic_cast<::term *>(positional_vals->at(2));

			::term * conc = new term(L1->make_str() + L2->make_str());
			frame_item * r = f->copy();
			if (conc->unify(r, L3))
				result = r;
			else {
				delete r;
			}
			conc->free();
			internal_variant++;
		}
		if (d1 && d2 && !d3 && internal_variant == 0) {
			::term * L1 = dynamic_cast<::term *>(positional_vals->at(0));
			::term * L2 = dynamic_cast<::term *>(positional_vals->at(1));
			value * L3 = dynamic_cast<::value *>(positional_vals->at(2));

			::term * conc = new term(L1->make_str() + L2->make_str());
			frame_item * r = f->copy();
			if (L3->unify(r, conc))
				result = r;
			else {
				delete r;
			}
			conc->free();
			internal_variant++;
		}
		if (d1 && !d2 && d3) {
			::term * L1 = dynamic_cast<::term *>(positional_vals->at(0));
			value * L2 = dynamic_cast<::value *>(positional_vals->at(1));
			::term * L3 = dynamic_cast<::term *>(positional_vals->at(2));

			string LL3S = L3->make_str();

			for (; !result && internal_variant <= LL3S.length(); internal_variant++) {
				value * K1 = new term(LL3S.substr(0, internal_variant));
				value * K2 = new term(LL3S.substr(internal_variant, LL3S.length() - internal_variant));

				frame_item * r = f->copy();

				if (L1->unify(r, K1) && L2->unify(r, K2))
					result = r;
				else {
					delete r;
				}
				K1->free();
				K2->free();
			}
		}
		if (!d1 && d2 && d3) {
			value * L1 = dynamic_cast<::value *>(positional_vals->at(0));
			::term * L2 = dynamic_cast<::term *>(positional_vals->at(1));
			::term * L3 = dynamic_cast<::term *>(positional_vals->at(2));

			string LL3S = L3->make_str();

			for (; !result && internal_variant <= LL3S.length(); internal_variant++) {
				value * K1 = new term(LL3S.substr(0, internal_variant));
				value * K2 = new term(LL3S.substr(internal_variant, LL3S.length() - internal_variant));

				frame_item * r = f->copy();

				if (L1->unify(r, K1) && L2->unify(r, K2))
					result = r;
				else {
					delete r;
				}
				K1->free();
				K2->free();
			}
		}
		if (!d1 && !d2 && d3) {
			value * L1 = dynamic_cast<::value *>(positional_vals->at(0));
			value * L2 = dynamic_cast<::value *>(positional_vals->at(1));
			::term * L3 = dynamic_cast<::term *>(positional_vals->at(2));

			string LL3S = L3->make_str();

			for (; !result && internal_variant <= LL3S.length(); internal_variant++) {
				value * K1 = new term(LL3S.substr(0, internal_variant));
				value * K2 = new term(LL3S.substr(internal_variant, LL3S.length() - internal_variant));

				frame_item * r = f->copy();

				if (L1->unify(r, K1) && L2->unify(r, K2))
					result = r;
				else {
					delete r;
				}
				K1->free();
				K2->free();
			}
		}

		return result;
	}

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 3) {
			std::cout << "atom_concat(A,B,C) incorrect call!" << endl;
			exit(-3);
		}
		d1 = positional_vals->at(0)->defined();
		d2 = positional_vals->at(1)->defined();
		d3 = positional_vals->at(2)->defined();
		if (!d1 && !d2 && !d3) {
			std::cout << "atom_concat(A,B,C) indeterminated!" << endl;
			exit(-3);
		}

		int internal_ptr = 0;
		frame_item * first = get_next_variant(f, internal_ptr, positional_vals);

		if (first)
			return new lazy_generated_vars(f, this, positional_vals, first, internal_ptr, once ? 1 : 0xFFFFFFFF);
		else
			return NULL;
	}
};

class predicate_item_atom_chars : public predicate_item {
public:
	predicate_item_atom_chars(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "atom_chars"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "atom_chars(A,B) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1 && !d2) {
			std::cout << "atom_chars(A,B) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		if (d1 && d2) {
			::term * A1 = dynamic_cast<::term *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));

			string S2 = L2->make_str();

			frame_item * r = f->copy();
			if (A1->make_str() == S2)
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
		}
		if (d1 && !d2) {
			::term * A1 = dynamic_cast<::term *>(positional_vals->at(0));
			::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));
			value * L2 = dynamic_cast<::value *>(positional_vals->at(1));

			frame_item * r = f->copy();

			::list * L = new ::list(std::list<value *>(), NULL);

			if (L1 && L1->size() == 0) {
				char buf1[2] = "[";
				char buf2[2] = "]";
				L->add(new ::term(buf1));
				L->add(new ::term(buf2));
			}
			else {
				string S = A1->make_str();
				for (char c : S) {
					char buf[2] = { c, 0 };
					L->add(new ::term(buf));
				}
			}

			if (L2->unify(r, L))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			L->free();
		}
		if (!d1 && d2) {
			value * A1 = dynamic_cast<::value *>(positional_vals->at(0));
			::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));

			frame_item * r = f->copy();

			string S = L2->make_str();
			term * tt = new ::term(S);

			if (A1->unify(r, tt))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			tt->free();
		}

		return result;
	}
};

class predicate_item_number_atom : public predicate_item {
public:
	predicate_item_number_atom(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "number_atom"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "number_atom(A,B) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1 && !d2) {
			std::cout << "number_atom(A,B) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		if (d1 && d2) {
			::int_number * N1 = dynamic_cast<::int_number *>(positional_vals->at(0));
			::float_number * F1 = dynamic_cast<::float_number *>(positional_vals->at(0));
			::term * A2 = dynamic_cast<::term *>(positional_vals->at(1));

			frame_item * r = f->copy();
			if (F1 && F1->make_str() == A2->make_str() || N1 && N1->make_str() == A2->make_str())
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
		}
		if (d1 && !d2) {
			::int_number * N1 = dynamic_cast<::int_number *>(positional_vals->at(0));
			::float_number * F1 = dynamic_cast<::float_number *>(positional_vals->at(0));
			value * A2 = dynamic_cast<::value *>(positional_vals->at(1));

			frame_item * r = f->copy();

			::term * S = new ::term(F1 ? F1->make_str() : N1->make_str());

			if (A2->unify(r, S))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			S->free();
		}
		if (!d1 && d2) {
			value * N1 = dynamic_cast<::value *>(positional_vals->at(0));
			::term * A2 = dynamic_cast<::term *>(positional_vals->at(1));

			frame_item * r = f->copy();

			int p = 0;
			value * S = prolog->parse(false, true, r, A2->make_str(), p);

			if ((dynamic_cast<int_number *>(S) || dynamic_cast<float_number *>(S)) && N1->unify(r, S))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			S->free();
		}

		return result;
	}
};

class predicate_item_number : public predicate_item {
public:
	predicate_item_number(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "number"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "number(A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "number(A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		if (dynamic_cast<int_number *>(positional_vals->at(0)) || dynamic_cast<float_number *>(positional_vals->at(0)))
			result->push_back(r);
		else {
			delete result;
			result = NULL;
			delete r;
		}

		return result;
	}
};

class predicate_item_open_url : public predicate_item {
public:
	predicate_item_open_url(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "open_url"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "open_url(URL|URLList) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "open_url(URL|URLList) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);

		::list * L = dynamic_cast<::list *>(positional_vals->at(0));
		string cmd;
		unsigned int ret = 0;
		if (L) {
			L->iterate([&](value * v) {
#ifdef __linux__
				cmd = "xdg-open ";
#else
				cmd = "start ";
#endif
				cmd += v->make_str();
				ret += abs(system(cmd.c_str()));
			});
		}
		else {
#ifdef __linux__
			cmd = "xdg-open ";
#else
			cmd = "start ";
#endif
			cmd += positional_vals->at(0)->make_str();
			ret += abs(system(cmd.c_str()));
		}

		if (ret != 0) {
			delete r;
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_track_post : public predicate_item {
public:
	predicate_item_track_post(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "track_post"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "track_post(ID|IDList) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "track_post(ID|IDList) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);

		::list * L = dynamic_cast<::list *>(positional_vals->at(0));
		unsigned int ret = 0;
		string cmd;
		if (L) {
			L->iterate([&](value * v) {
#ifdef __linux__
				cmd = "xdg-open ";
#else
				cmd = "start ";
#endif
				cmd += "https://gdeposylka.ru/";
				cmd += v->make_str();
				ret += abs(system(cmd.c_str()));
			});
		}
		else {
#ifdef __linux__
			cmd = "xdg-open ";
#else
			cmd = "start ";
#endif
			cmd += "https://gdeposylka.ru/";
			cmd += positional_vals->at(0)->make_str();
			ret += abs(system(cmd.c_str()));
		}

		if (ret != 0) {
			delete r;
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_consistency : public predicate_item {
public:
	predicate_item_consistency(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "consistency"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "consistency(PrefixesList) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "consistency(PrefixesList) indeterminated!" << endl;
			exit(-3);
		}
		::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));
		if (!L1) {
			std::cout << "consistency(PrefixesList) has incorrect parameter!" << endl;
			exit(-3);
		}

		set<string> L;
		auto push = [&](value * v) {
			term * t = dynamic_cast<term *>(v);
			if (t && t->get_args().size() == 0)
				L.insert(t->get_name());
		};
		L1->iterate(push);

		generated_vars * result = new generated_vars();
		if (prolog->check_consistency(L))
			result->push_back(f->copy());
		else {
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_last : public predicate_item {
public:
	predicate_item_last(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "last"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "last(L,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "last(L,A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));

		if (!L1) {
			std::cout << "last(L,A) : L is not a list!" << endl;
			exit(-3);
		}

		::value * V2 = dynamic_cast<::value *>(positional_vals->at(1));
		::value * LAST = ((::list *)L1)->get_last();

		if (LAST) {
			frame_item * r = f->copy();
			if (V2->unify(r, LAST))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			LAST->free();
		}
		else {
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_member : public predicate_item {
	bool d1;
	::list * L2;
public:
	predicate_item_member(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "member"; }

	virtual frame_item * get_next_variant(frame_item * f, int & internal_variant, vector<value *> * positional_vals) {
		frame_item * result = NULL;

		::value * V1 = dynamic_cast<::value *>(positional_vals->at(0));

		for (; !result && internal_variant < L2->size(); internal_variant++) {
			frame_item * r = f->copy();

			if (L2->get_nth(internal_variant + 1)->unify(r, V1))
				result = r;
			else
				delete r;
		}

		return result;
	}

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "member(A,L) incorrect call!" << endl;
			exit(-3);
		}
		d1 = positional_vals->at(0)->defined();

		L2 = dynamic_cast<::list *>(positional_vals->at(1));

		if (!L2) {
			return NULL;
		}

		int internal_ptr = 0;
		frame_item * first = get_next_variant(f, internal_ptr, positional_vals);

		if (first)
			return new lazy_generated_vars(f, this, positional_vals, first, internal_ptr, once ? 1 : 0xFFFFFFFF);
		else
			return NULL;
	}
};

class predicate_item_length : public predicate_item {
public:
	predicate_item_length(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "length"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "length(L,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "length(L,A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::list * L1 = dynamic_cast<::list *>(positional_vals->at(0));

		if (!L1) {
			std::cout << "length(L,A) : L is not a list!" << endl;
			exit(-3);
		}

		::value * V2 = dynamic_cast<::value *>(positional_vals->at(1));
		::value * L = new int_number(L1->size());

		if (L) {
			frame_item * r = f->copy();
			if (V2->unify(r, L))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			L->free();
		}
		else {
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_atom_length : public predicate_item {
public:
	predicate_item_atom_length(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "atom_length"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "atom_length(A,N) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "atom_length(A,N) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::term * T1 = dynamic_cast<::term *>(positional_vals->at(0));

		if (!T1 || T1->get_args().size() > 0) {
			std::cout << "atom_length(A,N) : A is not a simple atom!" << endl;
			exit(-3);
		}

		::value * V2 = dynamic_cast<::value *>(positional_vals->at(1));
		::value * L = new int_number(T1->get_name().length());

		if (L) {
			frame_item * r = f->copy();
			if (V2->unify(r, L))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			L->free();
		}
		else {
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_nth : public predicate_item {
public:
	predicate_item_nth(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "nth"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 3) {
			std::cout << "nth(Index,L,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		bool d3 = positional_vals->at(2)->defined();
		if (!d2 || !d1 && !d3) {
			std::cout << "nth(Index,L,A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::value * V1 = dynamic_cast<::value *>(positional_vals->at(0));
		::any * ANY1 = dynamic_cast<::any *>(positional_vals->at(0));
		::var * VAR1 = dynamic_cast<::var *>(positional_vals->at(0));
		::int_number * NUM1 = dynamic_cast<::int_number *>(positional_vals->at(0));
		::list * L2 = dynamic_cast<::list *>(positional_vals->at(1));
		::value * V3 = dynamic_cast<::value *>(positional_vals->at(2));

		if (!L2 || !ANY1 && !VAR1 && !NUM1) {
			std::cout << "nth(Index,L,A) : incorrect params!" << endl;
			exit(-3);
		}

		frame_item * r = f->copy();

		if (ANY1 || VAR1) { // Search
			int n = 1;
			auto check = [&](value * item) {
				if (V3->unify(r, item)) {
					::int_number * NN = new ::int_number(n);
					if (V1->unify(r, NN)) {
						result->push_back(r);
						r = f->copy();
					}
					NN->free();
				}
				n++;
			};
			L2->iterate(check);
			if (result->size() == 0) {
				delete result;
				result = NULL;
			}
			delete r;
		}
		else { // Unify nth-element
			value * V = L2->get_nth(NUM1->get_value());
			if (V && V3->unify(r, V))
				result->push_back(r);
			else {
				delete result;
				result = NULL;
				delete r;
			}
			V->free();
		}

		if (result && once && result->size() > 1) {
			for (int i = 1; i < result->size(); i++)
				delete result->at(i);
			result->resize(1);
		}

		return result;
	}
};

class predicate_item_listing : public predicate_item {
public:
	predicate_item_listing(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "listing"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "listing(A/n) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		indicator * t = dynamic_cast<indicator *>(positional_vals->at(0));
		if (!d1) {
			std::cout << "listing(A/n) indeterminated!" << endl;
			exit(-3);
		}
		if (!t) {
			std::cout << "listing(A/n) has incorrect parameter!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();
		result->push_back(r);

		map< string, vector<term *> *>::iterator it = prolog->DB.begin();
		while (it != prolog->DB.end()) {
			if (it->first == t->get_name())
				for (int i = 0; i < it->second->size(); i++)
					if (it->second->at(i)->get_args().size() == t->get_arity()) {
						it->second->at(i)->write(prolog->outs);
						(*prolog->outs) << "." << endl;
					}
			it++;
		}

		return result;
	}
};

class predicate_item_current_predicate : public predicate_item {
public:
	predicate_item_current_predicate(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "current_predicate"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "current_predicate(A/n) incorrect call!" << endl;
			exit(-3);
		}

		bool d1 = positional_vals->at(0)->defined();

		generated_vars * result = new generated_vars();

		if (d1) {
			indicator * t = dynamic_cast<indicator *>(positional_vals->at(0));
			if (!t) {
				std::cout << "current_predicate(A/n) has incorrect parameter!" << endl;
				exit(-3);
			}
			map< string, vector<term *> *>::iterator it = prolog->DB.begin();
			while (it != prolog->DB.end()) {
				if (it->first == t->get_name())
					for (int i = 0; i < it->second->size(); i++)
						if (it->second->at(i)->get_args().size() == t->get_arity()) {
							frame_item * r = f->copy();
							result->push_back(r);

							return result;
						}
				it++;
			}
			delete result;

			return NULL;
		}

		map< string, vector<term *> *>::iterator it = prolog->DB.begin();
		value * t = dynamic_cast<value *>(positional_vals->at(0));
		while (it != prolog->DB.end()) {
			set<int> seen;
			for (int i = 0; i < it->second->size(); i++) {
				int n = it->second->at(i)->get_args().size();
				if (seen.find(n) == seen.end()) {
					frame_item * r = f->copy();
					indicator * tt = new indicator(it->first, n);
					if (t->unify(r, tt)) {
						result->push_back(r);
					}
					else
						delete r;
					tt->free();
					seen.insert(n);
				}
			}
			it++;
		}
		if (result->size() == 0) {
			delete result;
			result = NULL;
		}

		if (result && once && result->size() > 1) {
			for (int i = 1; i < result->size(); i++)
				delete result->at(i);
			result->resize(1);
		}

		return result;
	}
};

class predicate_item_internal_predicate_property : public predicate_item {
public:
	predicate_item_internal_predicate_property(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "predicate_property"; }

	virtual generated_vars * _generate_variants(frame_item * f, term * signature, value * & prop) {
		generated_vars * result = new generated_vars();

		map< string, set<int> *>::iterator it = prolog->DBIndicators.find(signature->get_name());
		term * prop_val = new term("dynamic");
		int n = signature->get_args().size();
		if (it != prolog->DBIndicators.end()) {
			if (it->second->find(n) != it->second->end()) {
				frame_item * r = f->copy();
				if (prop->unify(r, prop_val)) {
					result->push_back(r);
				}
				else
					delete r;
			}
		}
		prop_val->free();
		if (result->size() == 0) {
			delete result;
			result = NULL;
		}

		return result;
	}
};

class predicate_item_predicate_property : public predicate_item_internal_predicate_property {
public:
	predicate_item_predicate_property(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item_internal_predicate_property(_neg, _once, _call, num, c, _prolog) { }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "predicate_property(pred(_,...,_),Prop) incorrect call!" << endl;
			exit(-3);
		}

		term * tt = dynamic_cast<term *>(positional_vals->at(0));
		if (tt) {
			term * functor = new term(tt->get_name());
			for (int i = 0; i < tt->get_args().size(); i++)
				functor->add_arg(f, new any());
			generated_vars * result = _generate_variants(f, functor, positional_vals->at(1));
			functor->free();
			return result;
		}

		return NULL;
	}
};

class predicate_item_predicate_property_pi : public predicate_item_internal_predicate_property {
public:
	predicate_item_predicate_property_pi(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item_internal_predicate_property(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "$predicate_property_pi"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "'$predicate_property_pi'(A/n,Prop) incorrect call!" << endl;
			exit(-3);
		}

		indicator * tt = dynamic_cast<indicator *>(positional_vals->at(0));
		if (tt) {
			term * t = new term(tt->get_name());
			for (int i = 0; i < tt->get_arity(); i++)
				t->add_arg(f, new any());
			generated_vars * result = _generate_variants(f, t, positional_vals->at(1));
			t->free();
			return result;
		}

		return NULL;
	}
};

class predicate_item_consult : public predicate_item {
public:
	predicate_item_consult(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "consult"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "consult(FName) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "consult(FName) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();

		prolog->consult(positional_vals->at(0)->make_str(), true);

		result->push_back(r);

		return result;
	}
};

class predicate_item_open : public predicate_item {
public:
	predicate_item_open(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "open"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 3) {
			std::cout << "open(fName,mode,S) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		bool d3 = positional_vals->at(2)->defined();
		if (!d1 || !d2 || d3) {
			std::cout << "open(fName,mode,S) incorrect params!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::term * fName = dynamic_cast<::term *>(positional_vals->at(0));
		::term * mode = dynamic_cast<::term *>(positional_vals->at(1));
		::value * S = dynamic_cast<::value *>(positional_vals->at(2));

		::term * id = new term(prolog->open_file(fName->make_str(), mode->make_str()));
		frame_item * r = f->copy();
		if (S->unify(r, id))
			result->push_back(r);
		else {
			delete result;
			result = NULL;
			delete r;
		}
		id->free();
		return result;
	}
};

class predicate_item_close : public predicate_item {
public:
	predicate_item_close(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "close"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "close(S) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "close(S) : undefined parameter!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		::term * S = dynamic_cast<::term *>(positional_vals->at(0));

		prolog->close_file(S->make_str());
		frame_item * r = f->copy();
		result->push_back(r);

		return result;
	}
};

class predicate_item_write : public predicate_item {
public:
	predicate_item_write(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "write"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() < 1 || positional_vals->size() > 2) {
			std::cout << "write(A)/write(S,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1 || positional_vals->size() == 2 && !positional_vals->at(1)->defined()) {
			std::cout << "write(A)/write(S,A) indeterminated!" << endl;
			exit(-3);
		}
		if (positional_vals->size() == 1)
			positional_vals->at(0)->write(prolog->outs);
		else {
			::term * S = dynamic_cast<::term *>(positional_vals->at(0));
			int fn;
			prolog->get_file(S->make_str(), fn) << positional_vals->at(1)->to_str();
		}

		generated_vars * result = new generated_vars();

		result->push_back(f->copy());

		return result;
	}
};

class predicate_item_nl : public predicate_item {
public:
	predicate_item_nl(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "nl"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 1) {
			std::cout << "nl/nl(S) incorrect call!" << endl;
			exit(-3);
		}

		if (positional_vals->size() == 0)
			(*prolog->outs) << endl;
		else {
			::term * S = dynamic_cast<::term *>(positional_vals->at(0));
			int fn;
			prolog->get_file(S->make_str(), fn) << endl;
		}

		generated_vars * result = new generated_vars();

		result->push_back(f->copy());

		return result;
	}
};

class predicate_item_tell : public predicate_item {
public:
	predicate_item_tell(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "tell"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 1) {
			std::cout << "tell(FName) incorrect call!" << endl;
			exit(-3);
		}

		if (!positional_vals->at(0)->defined()) {
			std::cout << "tell(FName) indeterminated!" << endl;
			exit(-3);
		}

		prolog->current_output = positional_vals->at(0)->make_str();
		prolog->outs = new ofstream(prolog->current_output, ios::out | ios::trunc);

		generated_vars * result = new generated_vars();

		result->push_back(f->copy());

		return result;
	}
};

class predicate_item_see : public predicate_item {
public:
	predicate_item_see(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "see"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 1) {
			std::cout << "see(FName) incorrect call!" << endl;
			exit(-3);
		}

		if (!positional_vals->at(0)->defined()) {
			std::cout << "see(FName) indeterminated!" << endl;
			exit(-3);
		}

		prolog->current_input = positional_vals->at(0)->make_str();
		prolog->ins = new ifstream(prolog->current_output, ios::in);

		generated_vars * result = new generated_vars();

		result->push_back(f->copy());

		return result;
	}
};

class predicate_item_telling : public predicate_item {
public:
	predicate_item_telling(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "telling"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 1) {
			std::cout << "telling(FName) incorrect call!" << endl;
			exit(-3);
		}

		::term * S = new term(prolog->current_output);

		generated_vars * result = new generated_vars();
		frame_item * ff = f->copy();

		if (positional_vals->at(0)->unify(ff, S))
			result->push_back(ff);
		else {
			delete ff;
			delete result;
			result = NULL;
		}

		S->free();

		return result;
	}
};

class predicate_item_seeing : public predicate_item {
public:
	predicate_item_seeing(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "seeing"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 1) {
			std::cout << "seeing(FName) incorrect call!" << endl;
			exit(-3);
		}

		::term * S = new term(prolog->current_input);

		generated_vars * result = new generated_vars();
		frame_item * ff = f->copy();

		if (positional_vals->at(0)->unify(ff, S))
			result->push_back(ff);
		else {
			delete ff;
			delete result;
			result = NULL;
		}

		S->free();

		return result;
	}
};

class predicate_item_told : public predicate_item {
public:
	predicate_item_told(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "told"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 0) {
			std::cout << "told/0 incorrect call!" << endl;
			exit(-3);
		}

		if (prolog->current_output != STD_OUTPUT)
			dynamic_cast<basic_ofstream<char> *>(prolog->outs)->close();
		prolog->outs = &std::cout;
		prolog->current_output = STD_OUTPUT;

		generated_vars * result = new generated_vars();
		frame_item * ff = f->copy();

		result->push_back(ff);

		return result;
	}
};

class predicate_item_seen : public predicate_item {
public:
	predicate_item_seen(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) : predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "seen"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() > 0) {
			std::cout << "seen/0 incorrect call!" << endl;
			exit(-3);
		}

		if (prolog->current_input != STD_INPUT)
			dynamic_cast<basic_ifstream<char> *>(prolog->ins)->close();
		prolog->ins = &cin;
		prolog->current_input = STD_INPUT;

		generated_vars * result = new generated_vars();
		frame_item * ff = f->copy();

		result->push_back(ff);

		return result;
	}
};

class predicate_item_get_or_peek_char : public predicate_item {
	bool peek;
public:
	predicate_item_get_or_peek_char(bool _peek, bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog), peek(_peek) { }

	virtual const string get_id() { return peek ? "peek_char" : "get_char"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << (peek ? "peek" : "get") << "_char(S,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1) {
			std::cout << (peek ? "peek" : "get") << "_char(S,A) : S is indeterminated!" << endl;
			exit(-3);
		}

		::term * S = dynamic_cast<::term *>(positional_vals->at(0));

		int fn;
		basic_fstream<char> & ff = prolog->get_file(S->make_str(), fn);

		char CS[2] = { 0 };
		value * v = NULL;
		if (peek) {
			int V = ff.peek();
			if (V == EOF) v = new term("end_of_file");
			else {
				CS[0] = V;
				v = new term(CS);
			}
		}
		else {
			ff.get(CS[0]);
			if (ff.eof()) v = new term("end_of_file");
			else
				v = new term(CS);
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();

		if (positional_vals->at(1)->unify(r, v))
			result->push_back(r);
		else {
			delete result;
			result = NULL;
			delete r;
		}
		v->free();

		return result;
	}
};

class predicate_item_read_token : public predicate_item {
	bool peek;
public:
	predicate_item_read_token(bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog) { }

	virtual const string get_id() { return "read_token"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 2) {
			std::cout << "read_token(S,A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		bool d2 = positional_vals->at(1)->defined();
		if (!d1) {
			std::cout << "read_token(S,A) : S is indeterminated!" << endl;
			exit(-3);
		}

		::term * S = dynamic_cast<::term *>(positional_vals->at(0));

		int fn;
		basic_fstream<char> & ff = prolog->get_file(S->make_str(), fn);

		streampos beg;
		string line;
		int p = 0;
		do {
			beg = ff.tellg();
			getline(ff, line);
			p = 0;
			prolog->bypass_spaces(line, p);
		} while (!ff.eof() && p == line.length());
		
		frame_item * dummy = new frame_item();
		value * v = NULL;
		if (p < line.length())
			if (line[p] >= '0' && line[p] <= '9') {
				v = prolog->parse(false, true, dummy, line, p);
				ff.seekg(beg + (streamoff)p);
			}
			else if (line[p] == '\'' || line[p] == '"') {
				v = prolog->parse(false, true, dummy, line, p);
				term * _v = new term("string");
				_v->add_arg(dummy, new term(unescape(v->make_str())));
				v->free();
				v = _v;
				ff.seekg(beg + (streamoff)p);
			}
			else if (line[p] >= 'A' && line[p] <= 'Z') {
				v = prolog->parse(false, true, dummy, line, p);
				term * _v = new term("var");
				_v->add_arg(dummy, new term(((var *)v)->get_name()));
				v->free();
				v = _v;
				ff.seekg(beg + (streamoff)p);
			}
			else if (line[p] != '(' && line[p] != ')' &&
				line[p] != '[' && line[p] != ']' && line[p] != '{' && line[p] != '}' &&
				line[p] != '|') {
				int st = p;
				if (line[p] == '_' || isalnum(line[p]))
					while (p < line.length() && (line[p] == '_' || isalnum(line[p])))
						p++;
				else
					p++;
				v = new term(line.substr(st, p - st));
				ff.seekg(beg + (streamoff)p);
			}
			else {
				char C[2] = { 0 };
				ff.seekg(beg + (streamoff)p);
				ff.get(C[0]);
				v = new term("punct");
				if (!ff.eof()) {
					((term *)v)->add_arg(dummy, new term(C));
				}
				else {
					((term *)v)->add_arg(dummy, new term("end_of_file"));
				}
			}
		else {
			v = new term("punct");
			((term *)v)->add_arg(dummy, new term("end_of_file"));
		}

		generated_vars * result = new generated_vars();
		frame_item * r = f->copy();

		delete dummy;

		if (v && positional_vals->at(1)->unify(r, v))
			result->push_back(r);
		else {
			delete result;
			result = NULL;
			delete r;
		}
		v->free();

		return result;
	}
};

class predicate_item_assert : public predicate_item {
	bool z;
public:
	predicate_item_assert(bool _z, bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog), z(_z) { }

	virtual const string get_id() { return z ? "assertz" : "asserta"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "assert(A) incorrect call!" << endl;
			exit(-3);
		}
		bool d1 = positional_vals->at(0)->defined();
		if (!d1) {
			std::cout << "assert(A) indeterminated!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		term * t = dynamic_cast<term *>(positional_vals->at(0));
		if (!t) {
			std::cout << "assert(A) : A is not a term!" << endl;
			exit(-3);
		}

		string atid = t->get_name();
		if (prolog->DB.find(atid) == prolog->DB.end())
			prolog->DB[atid] = new vector<term *>();
		
		vector<term *> * terms = prolog->DB[atid];
		if (z)
			terms->push_back((term *)t->copy(f));
		else
			terms->insert(terms->begin(), (term *)t->copy(f));

		if (prolog->DBIndicators.find(t->get_name()) == prolog->DBIndicators.end()) {
			set<int> * inds = new set<int>;
			inds->insert(t->get_args().size());
			prolog->DBIndicators[t->get_name()] = inds;
		}
		else {
			set<int> * inds = prolog->DBIndicators[t->get_name()];
			inds->insert(t->get_args().size());
		}

		result->push_back(f->copy());

		return result;
	}
};

class predicate_item_retract : public predicate_item {
	bool all;
public:
	predicate_item_retract(bool _all, bool _neg, bool _once, bool _call, int num, clause * c, interpreter * _prolog) :
		predicate_item(_neg, _once, _call, num, c, _prolog), all(_all) { }

	virtual const string get_id() { return all ? "retractall" : "retract"; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		if (positional_vals->size() != 1) {
			std::cout << "retract[all](A) incorrect call!" << endl;
			exit(-3);
		}

		generated_vars * result = new generated_vars();
		term * t = dynamic_cast<term *>(positional_vals->at(0));
		if (!t) {
			std::cout << "retract[all](A) : A is not a term!" << endl;
			exit(-3);
		}

		if (prolog->DB.find(t->get_name()) == prolog->DB.end())
			prolog->DB[t->get_name()] = new vector<term *>();

		vector<term *> * terms = prolog->DB[t->get_name()];
		vector<term *>::iterator it = terms->begin();
		while (it != terms->end()) {
			term * tt = (term *)t->copy(f);
			if (tt->unify(f, *it)) {
				if (all) it = terms->erase(it);
				else {
					frame_item * ff = f->copy();
					value * v1 = new int_number((long long)(*it));
					ff->set("$RETRACTOR$", v1);
					v1->free();
					value * v2 = new int_number((long long)terms);
					ff->set("$VECTOR$", v2);
					v2->free();
					result->push_back(ff);
					it++;
				}
			}
			else
				it++;
			tt->free();
		}

		if (all)
			result->push_back(f->copy());

		if (result && once && result->size() > 1) {
			for (int i = 1; i < result->size(); i++)
				delete result->at(i);
			result->resize(1);
		}

		return result;
	}

	virtual bool execute(frame_item * f) {
		if (all) return true;

		term * v = (term *)((long long)(0.5 + ((int_number *)f->get("$RETRACTOR$"))->get_value()));
		f->get("$RETRACTOR$")->free();
		vector<term *> * db = (vector<term *> *)((long long)(0.5+((int_number *)f->get("$VECTOR$"))->get_value()));
		f->get("$VECTOR$")->free();
		if (v && db) {
			vector<term *>::iterator it = find(db->begin(), db->end(), v);
			if (it != db->end())
				db->erase(it);
			return true;
		}
		else
			return false;
	}
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

	predicate * get_user_predicate() { return user_p; }

	virtual const string get_id() { return id; }

	virtual generated_vars * generate_variants(frame_item * f, vector<value *> * & positional_vals) {
		generated_vars * result = new generated_vars();
		if (user_p)
			for (int i = 0; i < user_p->num_clauses(); i++) {
				frame_item * r = f->copy();
				result->push_back(r);
			}
		else {
			if (prolog->DB.find(id) != prolog->DB.end()) {
				term * dummy = new term(id);
				for (int j = 0; j < positional_vals->size(); j++) {
					dummy->add_arg(f, positional_vals->at(j));
				}

				vector<term *> * terms = prolog->DB[id];
				for (int i = 0; i < terms->size(); i++) {
					frame_item * ff = f->copy();
					term * _dummy = (term *)dummy->copy(ff);
					if (_dummy->unify(ff, terms->at(i)))
						result->push_back(ff);
					else
						delete ff;
					_dummy->free();
				}
				dummy->free();
			}
			else {
				std::cout << "Predicate [" << id << "] is neither standard nor dynamic!" << endl;
				exit(500);
			}

			if (result && once && result->size() > 1) {
				for (int i = 1; i < result->size(); i++)
					delete result->at(i);
				result->resize(1);
			}

			if (result && result->size() == 0) {
				delete result;
				result = NULL;
			}
		}

		return result;
	}

	virtual bool processing(bool line_neg, int variant, generated_vars * variants, vector<value *> * & positional_vals, frame_item * up_f) {
		frame_item * f = new frame_item();
		predicate_item * next = get_next(variant);
		/**/
		if (built_flag){
			std::cout << this->get_id() << "[" << variant << "]" << endl;
		}
		if (variant == 0) {
			prolog->PARENT_CALLS.push(this);
			prolog->PARENT_CALL_VARIANT.push(0);
			prolog->CLAUSES.push(get_parent());
			prolog->CALLS.push(next);
			prolog->FRAMES.push(up_f->copy());
			prolog->NEGS.push(line_neg);
			prolog->_FLAGS.push((is_once() ? once_flag : 0) + (is_call() ? call_flag : 0));
		}

		prolog->PARENT_CALL_VARIANT.pop();
		prolog->PARENT_CALL_VARIANT.push(variant);

		if (prolog->retrieve(f, user_p->get_clause(variant), positional_vals, true)) {
			bool yes = user_p->get_clause(variant)->num_items() == 0;

			if (yes) {
				vector<value *> * v = NULL;
				yes = prolog->process(neg, user_p->get_clause(variant), NULL, f, v);
			}
			else {
				predicate_item * first = user_p->get_clause(variant)->get_item(0);
				vector<value *> * first_args = prolog->accept(f, first);
				yes = prolog->process(neg, user_p->get_clause(variant), first, f, first_args);
				for (int j = 0; j < first_args->size(); j++)
					first_args->at(j)->free();
				delete first_args;
			}

			if ((yes || (!variants || !variants->has_variant(variant+1))) && prolog->PARENT_CALLS.size() > 0 && prolog->PARENT_CALLS.top() == this) {
				prolog->CALLS.pop();
				prolog->PARENT_CALLS.pop();
				prolog->PARENT_CALL_VARIANT.pop();
				prolog->CLAUSES.pop();
				prolog->FRAMES.pop();
				prolog->NEGS.pop();
				prolog->_FLAGS.pop();
			}

			if (yes && prolog->PARENT_CALLS.size() == 0)
				up_f->sync(f);

			delete f;

			return yes;
		}
		else {
			if ((!variants || !variants->has_variant(variant + 1)) && prolog->PARENT_CALLS.size() > 0 && prolog->PARENT_CALLS.top() == this) {
				prolog->CALLS.pop();
				prolog->PARENT_CALLS.pop();
				prolog->PARENT_CALL_VARIANT.pop();
				prolog->CLAUSES.pop();
				prolog->FRAMES.pop();
				prolog->NEGS.pop();
				prolog->_FLAGS.pop();
			}

			delete f;

			return false;
		}
	}
};

void interpreter::block_process(bool clear_flag, bool cut_flag, predicate_item * frontier) {
	std::list<int>::iterator itf = FLAGS.end();
	bool passed_frontier = false;
	int nn = TRACE.size() - 1;
	while (nn >= 0) {
		generated_vars * T = TRACE[nn];
		predicate_item * WHO = TRACERS[nn];
		int n = ptrTRACE[nn];
		int flags = *(--itf);
		int level = LEVELS[nn];

		if (cut_flag && frontier && PARENT_CALLS.top()->get_parent() &&
			WHO && WHO->get_parent() == PARENT_CALLS.top()->get_parent() &&
			n == PARENT_CALL_VARIANT.top() &&
			level == PARENT_CALLS.size() - 1)
			break;

		if (cut_flag && T) {
			T->trunc_from(n + 1);
		}

		if ((flags & (call_flag | once_flag))) {
			if (clear_flag) *itf &= ~(call_flag | once_flag);
			break;
		}

		if (cut_flag && PARENT_CALLS.top()->get_parent() &&
			WHO && WHO->get_parent() == PARENT_CALLS.top()->get_parent() &&
			n == PARENT_CALL_VARIANT.top() &&
			level == PARENT_CALLS.size()-1)
			break;

		if (cut_flag && passed_frontier &&
			WHO && dynamic_cast<predicate_left_bracket *>(WHO) &&
			!dynamic_cast<predicate_left_bracket *>(WHO)->is_inserted() &&
			level == PARENT_CALLS.size())
			break;

		if (cut_flag && !passed_frontier && frontier &&
			WHO == frontier &&
			level == PARENT_CALLS.size())
			passed_frontier = true;

		nn--;
	}
}

bool interpreter::bypass_spaces(const string & s, int & p) {
	do {
		while (p < s.length() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r'))
			p++;
		if (p < s.length() && s[p] == '%') {
			string::size_type lpos = s.find('\n', p);
			if (lpos == string::npos)
				p = s.length();
			else
				p = lpos + 1;
		} else
			break;
	} while (true);
	return p < s.length();
}

value * interpreter::parse(bool exit_on_error, bool parse_complex_terms, frame_item * ff, const string & s, int & p) {
	if (p < s.length()) {
		value * result = NULL;

		if (!bypass_spaces(s, p)) return NULL;
		if (s[p] >= '0' && s[p] <= '9' || p + 1 < s.length() && (s[p] == '-' || s[p] == '+') && s[p+1] >= '0' && s[p+1] <= '9') {
			double v = 0;
			double sign = +1;
			bool flt = false;

			if (s[p] == '+') p++;
			else if (s[p] == '-') {
				sign = -1;
				p++;
			}

			while (p < s.length() && s[p] >= '0' && s[p] <= '9')
				v = v * 10 + (s[p++] - '0');
			if (s[p] == '.') {
				flt = true;
				p++;
				double q = 0.1;
				while (p < s.length() && s[p] >= '0' && s[p] <= '9') {
					v += q*(s[p++] - '0');
					q /= 10.0;
				}
			}
			if (s[p] == 'E' || s[p] == 'e') {
				flt = true;
				p++;
				long long n = 0;
				while (p < s.length() && s[p] >= '0' && s[p] <= '9')
					n = n * 10 + (s[p++] - '0');
				v *= pow(10.0, n);
			}
			if (flt)
				result = new float_number(sign*v);
			else
				result = new int_number(sign*((long long)(v + 0.5)));
		}
		else if (s[p] == '"' || s[p] == '\'') {
			int oldp = p++;
			string st;
			int n = -1;
			while (p < s.length() && (s[p] != s[oldp] || p+1<s.length() && s[p+1] == s[oldp])) {
				if (s[p] == '\\')
					st += s[p++];
				else if (s[p] == s[oldp])
					p++;
				st += s[p++];
			}
			p++;
			if (p < s.length()) {
				if (s[p] == '/') {
					p++;
					n = 0;
					while (p < s.length() && isdigit(s[p]))
						n = n * 10 + (s[p++] - '0');
				}
			}
			if (n < 0)
				result = new term(unescape(st));
			else
				result = new indicator(unescape(st), n);
		}
		else if (s[p] == '[') {
			result = new ::list(std::list<value *>(), NULL);
			p++;
			bypass_spaces(s, p);
			int oldp = p;
			while (p < s.length() && s[p] != ']' && s[p] != '|') {
				value * v = parse(exit_on_error, true, ff, s, p);
				if (p >= s.length() || v == NULL) {
					if (exit_on_error) {
						std::cout << "[" << s.substr(oldp, p - oldp) << "] : incorrect list!" << endl;
						exit(-1);
					}
					else
						return NULL;
				}
				((::list *)result)->add(v);
				if (!bypass_spaces(s, p)) {
					if (exit_on_error) {
						std::cout << "[" << s.substr(oldp, p - oldp) << "] : incorrect list!" << endl;
						exit(-1);
					}
					else
						return NULL;
				}
				if (s[p] == ',') p++;
				else if (s[p] != ']' && s[p] != '|') {
					if (exit_on_error) {
						std::cout << "[" << s.substr(oldp, p - oldp) << "] : incorrect list!" << endl;
						exit(-1);
					}
					else
						return NULL;
				}
			}
			if (s[p] == '|') {
				p++;
				value * t = parse(exit_on_error, true, ff, s, p);
				if (p >= s.length() || t == NULL || dynamic_cast<int_number *>(t) != NULL ||
					dynamic_cast<float_number *>(t) != NULL || dynamic_cast<term *>(t) != NULL ||
					dynamic_cast<indicator *>(t) != NULL) {
					if (exit_on_error) {
						std::cout << "[" << s.substr(oldp, p - oldp) << "] : incorrect tag of list!" << endl;
						exit(-1);
					}
					else
						return NULL;
				}
				((::list *)result)->set_tag(t);
				t->free();
				if (bypass_spaces(s, p) && s[p] == ']') p++;
				else {
					if (exit_on_error) {
						std::cout << "[" << s.substr(oldp, p - oldp) << "] : incorrect tag of list!" << endl;
						exit(-1);
					}
					else
						return NULL;
				}
			} else
				p++;
		}
		else if (s[p] >= 'A' && s[p] <= 'Z') {
			string st = string("") + s[p++];
			while (p < s.length() &&
				(s[p] >= 'A' && s[p] <= 'Z' || s[p] >= 'a' && s[p] <= 'z' || s[p] >= '0' && s[p] <= '9' ||
				 s[p] == '_'))
				st += s[p++];
			bool found = false;
			int it = ff->find(st.c_str(), found);
			if (found)
				result = ff->at(it)->copy(ff);
			else
				result = new var(st);
		}
		else if (s[p] == '_') {
			p++;
			result = new any();
		}
		else if (s[p] >= 'a' && s[p] <= 'z') {
			string st = string("") + s[p++];
			int n = -1;
			while (p < s.length() &&
				(s[p] >= 'A' && s[p] <= 'Z' || s[p] >= 'a' && s[p] <= 'z' || s[p] >= '0' && s[p] <= '9' ||
				s[p] == '_'))
				st += s[p++];

			if (p < s.length()) {
				if (s[p] == '/') {
					p++;
					n = 0;
					while (p < s.length() && isdigit(s[p]))
						n = n * 10 + (s[p++] - '0');
				}
			}

			if (n >= 0)
				result = new indicator(st, n);
			else {
				result = new term(st);

				if (parse_complex_terms && s[p] == '(') {
					p++;
					int oldp = p;
					bypass_spaces(s, p);
					while (p < s.length() && s[p] != ')') {
						value * v = parse(exit_on_error, true, ff, s, p);
						if (p >= s.length() || v == NULL) {
							if (exit_on_error) {
								std::cout << "(" << s.substr(oldp, p - oldp) << ") : incorrect term!" << endl;
								exit(-1);
							}
							else
								return NULL;
						}
						((term *)result)->add_arg(ff, v);
						v->free();
						if (!bypass_spaces(s, p)) {
							if (exit_on_error) {
								std::cout << "(" << s.substr(oldp, p - oldp) << ") : incorrect term!" << endl;
								exit(-1);
							}
							else
								return NULL;
						}
						if (s[p] == ',') p++;
						else if (s[p] != ')') {
							if (exit_on_error) {
								std::cout << "(" << s.substr(oldp, p - oldp) << ") : incorrect term!" << endl;
								exit(-1);
							}
							else
								return NULL;
						}
					}
					p++;
				}
			}
		}

		return result;
	}
	else {
		if (exit_on_error) {
			std::cout << "parse : syntax error!" << endl;
			exit(-2);
		}
		else
			return NULL;
	}
}

bool interpreter::unify(frame_item * ff, value * from, value * to) {
	return to->unify(ff, from);
}

vector<value *> * interpreter::accept(frame_item * ff, predicate_item * current) {
	vector<value *> * result = new vector<value *>();

	if (current->get_args()->size() > current->_get_args()->size())
		for (const string & s : *current->get_args()) {
			frame_item * empty = new frame_item();
			int p = 0;
			value * item = parse(true, true, empty, s, p);
			current->push_cashed_arg(item);
			value * _item = item->copy(ff);
			item->free();
			_item = _item->fill(ff); // ???????? item ?? ????????? ????????

			result->push_back(_item);
			delete empty;
		}
	else {
		for (value * v : *current->_get_args())
			result->push_back(v->copy(ff));
	}

	return result;
}

bool interpreter::retrieve(frame_item * ff, clause * current, vector<value *> * vals, bool escape_vars) {
	bool result = true;

	if (escape_vars)
		for (value * v : *vals)
			v->escape_vars(ff);

	int k = 0;
	if (current->get_args()->size() > current->_get_args()->size())
		for (const string & s : *current->get_args()) {
			frame_item * empty = new frame_item();
			int p = 0;
			value * item = parse(true, true, empty, s, p);
			current->push_cashed_arg(item);
			value * _item = item->copy(ff);
			item->free();
			if (!unify(ff, vals->at(k++), _item)) // ????????? item, ??????? ??????????? ??????????????? ??????????
				result = false;
			_item->free();
			delete empty;
		}
	else {
		for (value * proto : *current->_get_args()) {
			value * _item = proto->copy(ff);
			if (!unify(ff, vals->at(k++), _item)) // ????????? item, ??????? ??????????? ??????????????? ??????????
				result = false;
			_item->free();
		}
	}

	return result;
}

vector<value *> * interpreter::accept(frame_item * ff, clause * current) {
	vector<value *> * result = new vector<value *>();

	if (current->get_args()->size() > current->_get_args()->size())
		for (const string & s : *current->get_args()) {
			frame_item * empty = new frame_item();
			int p = 0;
			value * item = parse(true, true, empty, s, p);
			current->push_cashed_arg(item);
			value * _item = item->copy(ff);
			item->free();
			_item = _item->fill(ff); // ???????? item ?? ????????? ????????

			result->push_back(_item);
			delete empty;
		}
	else {
		for (value * v : *current->_get_args())
			result->push_back(v->copy(ff));
	}

	return result;
}

bool interpreter::retrieve(frame_item * ff, predicate_item * current, vector<value *> * vals, bool escape_vars) {
	bool result = true;

	if (escape_vars)
		for (value * v : *vals)
			v->escape_vars(ff);

	int k = 0;
	if (current->get_args()->size() > current->_get_args()->size())
		for (const string & s : *current->get_args()) {
			frame_item * empty = new frame_item();
			int p = 0;
			value * item = parse(true, true, empty, s, p);
			current->push_cashed_arg(item);
			value * _item = item->copy(ff);
			item->free();
			if (!unify(ff, vals->at(k++), _item)) // ????????? item, ??????? ??????????? ??????????????? ??????????
				result = false;
			_item->free();
			delete empty;
		}
	else {
		for (value * proto : *current->_get_args()) {
			value * _item = proto->copy(ff);
			if (!unify(ff, vals->at(k++), _item)) // ????????? item, ??????? ??????????? ??????????????? ??????????
				result = false;
			_item->free();
		}
	}

	return result;
}

bool interpreter::process(bool neg, clause * this_clause, predicate_item * p, frame_item * f, vector<value *> * & positional_vals) {
	predicate_item_user * user = dynamic_cast<predicate_item_user *>(p);

	generated_vars * variants = p ? p->generate_variants(f, positional_vals) : new generated_vars(1, f->copy());

	bool neg_standard = !(user && !user->is_dynamic()) && p && p->is_negated();
	int i = 0;
	/**/
	if (built_flag){
		std::cout << "entering process: ";
	}
	if (variants || neg_standard) {
		FLAGS.push_back((p && p->is_once() ? once_flag : 0) + (p && p->is_call() ? call_flag : 0));
		LEVELS.push(PARENT_CALLS.size());
		TRACE.push(variants);
		TRACERS.push(p);
		for (i = 0; neg_standard || variants && variants->has_variant(i); i++) {
			predicate_item * next = p ? p->get_next(i) : NULL;
			frame_item * ff = variants ? variants->get_next_variant(i) : f->copy();
			bool success = false;
			ptrTRACE.push(i);
			/**/
			if (built_flag){
				std::cout << (p ? p->get_id() : "END") << "[" << i << "]" << endl;
				cin.get();
			}
			if (user && !user->is_dynamic()) {
				if (!user->is_negated()) {
					if (user->processing(neg, i, variants, positional_vals, ff)) {
						/**/
						if (built_flag){
							std::cout << "true ret to " << (p ? p->get_id() : "END") << "[" << i << "]" << endl;
							//cin.get();
						}
						TRACE.pop();
						TRACERS.pop();
						FLAGS.pop_back();
						LEVELS.pop();
						ptrTRACE.pop();

						if (variants)
							variants->delete_from(i);
						else
							delete ff;

						delete variants;

						return true;
					}
					else if (!variants->has_variant(i+1)) {
						/**/
						if (built_flag){
							std::cout << "false ret to " << (p ? p->get_id() : "END") << "[" << i << "]" << endl;
							//cin.get();
						}
						ptrTRACE.pop();
						FLAGS.pop_back();
						LEVELS.pop();
						TRACE.pop();
						TRACERS.pop();

						delete variants;

						return false;
					}
				}
				else {
					if (user->processing(neg, i, variants, positional_vals, ff)) {
						TRACE.pop();
						TRACERS.pop();
						FLAGS.pop_back();
						LEVELS.pop();
						ptrTRACE.pop();

						if (variants)
							variants->delete_from(i);
						else
							delete ff;
						delete variants;

						return false;
					}
					else if (!variants->has_variant(i + 1)) {
						success = true;
					}
				}
			}
			else {
				if (neg_standard) {
					if (variants && variants->had_variants() && (!p || p->execute(ff))) {
						TRACE.pop();
						TRACERS.pop();
						FLAGS.pop_back();
						LEVELS.pop();
						ptrTRACE.pop();

						if (variants)
							variants->delete_from(i);
						else
							delete ff;
						delete variants;

						return false;
					}
					neg_standard = false;
				}
				success = !p || p->execute(ff);
			}

			if (success) {
				bool yes = next == NULL;

				predicate_right_bracket * rb = dynamic_cast<predicate_right_bracket *>(p);
				predicate_left_bracket * lb = dynamic_cast<predicate_left_bracket *>(p);
				bool rbc = rb && rb->get_corresponding()->is_call();
				bool rbo = rb && rb->get_corresponding()->is_once();

				if (rbc || rbo) {
					block_process(true, rbo, NULL);
				}
				else if (!user && !lb && p && (p->is_call() || p->is_once())) {
					FLAGS.back() &= ~(call_flag | once_flag);
					if (variants && p->is_once())
						variants->trunc_from(i+1);
				}

				if (!yes) {
					vector<value *> * next_args = accept(ff, next);
					if (next_args) {
						yes = process(neg, this_clause, next, ff, next_args);
						/**/
						if (built_flag){
							std::cout << (yes ? "true" : "false") << " ret to " << (p ? p->get_id() : "END") << "[" << i << "]" << endl;
							//cin.get();
						}

						for (int j = 0; j < next_args->size(); j++)
							next_args->at(j)->free();
						delete next_args;
						if (yes) {
							TRACE.pop();
							TRACERS.pop();
							FLAGS.pop_back();
							LEVELS.pop();
							ptrTRACE.pop();

							if (PARENT_CALLS.size() == 0)
								f->sync(ff);

							if (variants)
								variants->delete_from(i);
							else
								delete ff;
							delete variants;

							return true;
						}
					}
				}
				else {
					// f->sync(ff);

					predicate_item * next_clause_call = NULL;
					predicate_item * parent_call = NULL;
					clause * next_clause = NULL;
					bool next_neg = false;
					int _flags = 0;
					int old_variant = 0;
					bool got = CALLS.size() > 0;
					if (got) {
						next_clause_call = CALLS.top();
						CALLS.pop();
						next_clause = CLAUSES.top();
						CLAUSES.pop();
						parent_call = PARENT_CALLS.top();
						PARENT_CALLS.pop();
						old_variant = PARENT_CALL_VARIANT.top();
						PARENT_CALL_VARIANT.pop();
						next_neg = NEGS.top();
						NEGS.pop();
						_flags = _FLAGS.top();
						_FLAGS.pop();
					}

					if (got) {
						frame_item * up_ff = NULL;
						if (FRAMES.size() > 0) {
							up_ff = FRAMES.top();
							FRAMES.pop();
						}
						frame_item * _up_ff = !got /* next_clause == this_clause */ ? ff->copy() : up_ff->copy();

						if (this_clause) {
							vector<value *> * up_args = accept(ff, this_clause);
							if (up_args) {
								retrieve(_up_ff, parent_call, up_args, false);
								for (int j = 0; j < up_args->size(); j++)
									up_args->at(j)->free();
								delete up_args;
							}
						}

						if (neg) {
							TRACE.pop();
							TRACERS.pop();
							FLAGS.pop_back();
							LEVELS.pop();
							ptrTRACE.pop();

							if (variants)
								variants->delete_from(i);
							else
								delete ff;
							delete variants;
							delete _up_ff;

							return true;
						}

						if (_flags & (call_flag | once_flag)) {
							block_process(true, (_flags & once_flag) != 0, NULL);
						}

						vector<value *> * next_args =
							next_clause_call ? accept(_up_ff, next_clause_call) : new vector<value *>();
						if (next_args) {
							yes = process(next_neg, next_clause, next_clause_call, _up_ff, next_args);
							/**/
							if (built_flag){
								std::cout << (yes ? "true" : "false") << " ret to " << (p ? p->get_id() : "END") << "[" << i << "]" << endl;
								//cin.get();
							}
							if (!yes) {
								PARENT_CALLS.push(dynamic_cast<predicate_item_user *>(parent_call));
								PARENT_CALL_VARIANT.push(old_variant);
								CLAUSES.push(next_clause);
								CALLS.push(next_clause_call);
								FRAMES.push(up_ff);
								NEGS.push(next_neg);
								_FLAGS.push(_flags);
							}
							for (int j = 0; j < next_args->size(); j++)
								next_args->at(j)->free();
							delete next_args;
							if (yes) {
								TRACE.pop();
								TRACERS.pop();
								FLAGS.pop_back();
								LEVELS.pop();
								ptrTRACE.pop();

								if (variants)
									variants->delete_from(i);
								else
									delete ff;
								delete variants;
								delete _up_ff;

								return true;
							}
						}
						delete _up_ff;
					}
					else {
						TRACE.pop();
						TRACERS.pop();
						FLAGS.pop_back();
						LEVELS.pop();
						ptrTRACE.pop();

						if (variants)
							variants->delete_from(i);
						else
							delete ff;
						delete variants;

						return true;
					}
				}
			}
			ptrTRACE.pop();
			delete ff;
		}
		FLAGS.pop_back();
		LEVELS.pop();
		TRACE.pop();
		TRACERS.pop();
	}

	if (variants)
		variants->delete_from(i);
	delete variants;

	return false;
}

void interpreter::parse_clause(vector<string> & renew, frame_item * ff, string & s, int & p) {
	stack<predicate_left_bracket *> brackets;
	stack< stack<predicate_or *> > ors;
	stack<int> or_levels;
	bool or_branch = false;

	bypass_spaces(s, p);

	string id;
	while (p < s.length() &&
		(s[p] >= 'A' && s[p] <= 'Z' || s[p] >= 'a' && s[p] <= 'z' || s[p] >= '0' && s[p] <= '9' ||
		s[p] == '_'))
		id += s[p++];

	if (id.length() == 0) {
		std::cout << s.substr(p,10) << " : strange clause head!" << endl;
		exit(2);
	}

	predicate * pr = PREDICATES[id];

	vector<string>::iterator itr = find(renew.begin(), renew.end(), id);
	if (itr != renew.end()) {
		delete PREDICATES[id];
		PREDICATES.erase(id);
		renew.erase(itr);
		pr = NULL;
	}

	if (pr == NULL)
		PREDICATES[id] = pr = new predicate(id);

	clause * cl = new clause(pr);
	pr->add_clause(cl);

	if (p < s.length() && s[p] == '(') {
		p++;
		while (p < s.length() && s[p] != ')') {
			int oldp = p;
			value * dummy = parse(true, true, ff, s, p);
			if (dummy) {
				dummy->free();
				cl->add_arg(s.substr(oldp, p - oldp));
			}
			else {
				std::cout << id << " : strange clause head!" << endl;
				exit(2);
			}
			if (!bypass_spaces(s, p)) {
				std::cout << id << " : strange clause head!" << endl;
				exit(2);
			}
			if (s[p] == ',') p++;
			else if (s[p] != ')') {
				std::cout << id << " : unexpected '" << s[p] << "' in clause head!" << endl;
				exit(2);
			}
		}
		p++;
	}

	bypass_spaces(s, p);

	if (p < s.length() && s[p] == '.') {
		p++;
	}
	else if (p < s.length()-1 && s[p] == ':' && s[p+1] == '-') {
		int num = 0;
		p += 2;

		s.insert(s.begin() + p, '(');

		predicate_item * prev_pi = NULL;

		do {
			predicate_item * pi = NULL;

			bypass_spaces(s, p);

			bool or_request = false;

			bool neg = false;
			bool once = false;
			bool call = false;
			
			if (p + 1 < s.length() && s[p] == '\\' && s[p + 1] == '+') {
				neg = true;
				p += 2;
			}

			bypass_spaces(s, p);
			if (s.substr(p, 5) == "once(") {
				if (neg) {
					p += 4;
				}
				else {
					once = true;
					brackets.push(NULL);
					p += 5;
				}
			}
			else if (s.substr(p, 5) == "call(") {
				if (!neg) call = true;
				p += 4;
			}

			bypass_spaces(s, p);

			if (p + 1 < s.length() && s[p] == '\\' && s[p + 1] == '+') {
				neg = true;
				p += 2;
				if (once) {
					s.insert(p, "(");
					brackets.pop();
				}
				once = false;
			}

			bypass_spaces(s, p);
			if (p+2 < s.length() && s[p] == '\'' && s[p+1] == ',' && s[p+2] == '\'')
				p+=3;
			else if (p < s.length() && s[p] == ';') {
				or_request = true;
				p++;
			}

			bypass_spaces(s, p);
			if (s[p] == '!') {
				pi = new predicate_item_cut(NULL, num, cl, this);
				p++;
			}
			else if (s[p] == '(') {
				if (neg) {
					predicate_item * _pi = new predicate_left_bracket(false, false, false, true, num++, cl, this);
					if (or_branch) {
						brackets.top()->push_branch(_pi);
						or_branch = false;
					}
					cl->add_item(_pi);
					brackets.push((predicate_left_bracket *)_pi);
					ors.push(stack<predicate_or *>());
				}
				p++;
				pi = new predicate_left_bracket(brackets.size() == 0, neg, once, call, num, cl, this);
				if (or_branch) {
					brackets.top()->push_branch(pi);
					or_branch = false;
				}
				brackets.push((predicate_left_bracket *)pi);
				ors.push(stack<predicate_or *>());
				if (or_request) {
					or_levels.push(brackets.size());
					or_request = false;
				}
			}
			else if (s[p] == ')') {
				p++;
				if (!brackets.top()) {
					brackets.pop();
				}
				else {
					bool neg = brackets.top()->is_negated();
					brackets.top()->clear_negated();

					if (or_levels.size() > 0 && or_levels.top() == brackets.size()) {
						or_levels.pop();
					}

					pi = new predicate_right_bracket(brackets.top(), num, cl, this);
					if (or_branch) {
						brackets.top()->push_branch(pi);
						or_branch = false;
					}
					brackets.pop();
					stack<predicate_or *> & or_items = ors.top();
					while (!or_items.empty()) {
						or_items.top()->set_ending(pi);
						or_items.pop();
					}
					ors.pop();

					if (neg) {
						cl->add_item(pi);
						num++;
						predicate_right_bracket * _pi = new predicate_right_bracket(brackets.top(), num + 4, cl, this);
						pi = new predicate_item_cut(NULL, num++, cl, this);
						cl->add_item(pi);
						pi = new predicate_item_fail(false, false, false, num++, cl, this);
						cl->add_item(pi);
						pi = new predicate_or(num++, cl, this);
						ors.top().push((predicate_or *)pi);

						cl->add_item(pi);
						pi = new predicate_item_true(false, false, false, num++, cl, this);
						cl->add_item(pi);
						brackets.top()->push_branch(pi);

						stack<predicate_or *> & or_items = ors.top();
						while (!or_items.empty()) {
							or_items.top()->set_ending(_pi);
							or_items.pop();
						}
						brackets.pop();
						ors.pop();

						pi = _pi;
					}
				}
			}
			else {
				string iid;
				bypass_spaces(s, p);

				if (p + 2 < s.length() && s[p] == '=' && s[p + 1] == '.' && s[p + 2] == '.') {
					iid = "term_split";
					p += 3;
				}
				else if (p+1 < s.length() && s[p] == '=' && s[p+1] == '=') {
					iid = "eq";
					p+=2;
				}
				else if (p < s.length() && s[p] == '=') {
					iid = "eq";
					p++;
				}
				else if (p < s.length() && s[p] == '<') {
					iid = "less";
					p++;
				}
				else if (p < s.length() && s[p] == '>') {
					iid = "greater";
					p++;
				}
				else if (p + 1 < s.length() && s[p] == '\\' && s[p + 1] == '=') {
					iid = "neq";
					p += 2;
				}
				else {
					int start = p;
					value * dummy = parse(false, false, ff, s, p);
					iid = s.substr(start, p - start);
					dummy->free();
				}

				if (iid.length() == 0) {
					std::cout << id << " : " << s.substr(p,10) << " : strange item call!" << endl;
					exit(2);
				}

				bypass_spaces(s, p);
				if (s.substr(p, 2) == "is" || p < s.length() && (s[p] == '>' || s[p] == '<')) {
					string op = s[p] == '>' ? "greater" : (s[p] == '<' ? "less" : "is");
					if (op == "is")
						p += 2;
					else
						p++;
					bypass_spaces(s, p);
					string expr_chars = "+-*/\\()<>^=!";
					string expr;
					int br_level = 0;
					while (p < s.length() && (s[p] == '_' || isspace(s[p]) || isalnum(s[p]) || expr_chars.find(s[p]) != string::npos) ||
						s[p] == ',' && br_level ||
						   p+1 < s.length() && s[p] == '.' && isdigit(s[p+1])) {
						if (s[p] == '(') br_level++;
						else if (s[p] == ')')
							if (br_level)
								br_level--;
							else
								break;
						expr += s[p];
						p++;
					}
					if (op == "is")
						pi = new predicate_item_is(iid, expr, neg, once, call, num, cl, this);
					else if (op == "less") {
						pi = new predicate_item_less(neg, once, call, num, cl, this);
						pi->add_arg(iid);
						pi->add_arg(expr);
					}
					else if (op == "greater") {
						pi = new predicate_item_greater(neg, once, call, num, cl, this);
						pi->add_arg(iid);
						pi->add_arg(expr);
					}
				}
				else if (iid == "append") {
					pi = new predicate_item_append(neg, once, call, num, cl, this);
				}
				else if (iid == "member") {
					pi = new predicate_item_member(neg, once, call, num, cl, this);
				}
				else if (iid == "last") {
					pi = new predicate_item_last(neg, once, call, num, cl, this);
				}
				else if (iid == "length") {
					pi = new predicate_item_length(neg, once, call, num, cl, this);
				}
				else if (iid == "atom_length") {
					pi = new predicate_item_atom_length(neg, once, call, num, cl, this);
				}
				else if (iid == "nth") {
					pi = new predicate_item_nth(neg, once, call, num, cl, this);
				}
				else if (iid == "atom_concat") {
					pi = new predicate_item_atom_concat(neg, once, call, num, cl, this);
				}
				else if (iid == "atom_chars") {
					pi = new predicate_item_atom_chars(neg, once, call, num, cl, this);
				}
				else if (iid == "number_atom") {
					pi = new predicate_item_number_atom(neg, once, call, num, cl, this);
				}
				else if (iid == "number") {
					pi = new predicate_item_number(neg, once, call, num, cl, this);
				}
				else if (iid == "consistency") {
					pi = new predicate_item_consistency(neg, once, call, num, cl, this);
				}
				else if (iid == "listing") {
					pi = new predicate_item_listing(neg, once, call, num, cl, this);
				}
				else if (iid == "current_predicate") {
					pi = new predicate_item_current_predicate(neg, once, call, num, cl, this);
				}
				else if (iid == "predicate_property") {
					pi = new predicate_item_predicate_property(neg, once, call, num, cl, this);
				}
				else if (iid == "$predicate_property_pi") {
					pi = new predicate_item_predicate_property_pi(neg, once, call, num, cl, this);
				}
				else if (iid == "eq" || iid == "=") {
					pi = new predicate_item_eq(neg, once, call, num, cl, this);
				}
				else if (iid == "neq" || iid == "\\=") {
					pi = new predicate_item_neq(neg, once, call, num, cl, this);
				}
				else if (iid == "less" || iid == "<") {
					pi = new predicate_item_less(neg, once, call, num, cl, this);
				}
				else if (iid == "greater" || iid == ">") {
					pi = new predicate_item_greater(neg, once, call, num, cl, this);
				}
				else if (iid == "term_split" || iid == "..") {
					pi = new predicate_item_term_split(neg, once, call, num, cl, this);
				}
				else if (iid == "g_assign") {
					pi = new predicate_item_g_assign(neg, once, call, num, cl, this);
				}
				else if (iid == "g_read") {
					pi = new predicate_item_g_read(neg, once, call, num, cl, this);
				}
				else if (iid == "fail") {
					pi = new predicate_item_fail(neg, once, call, num, cl, this);
				}
				else if (iid == "true") {
					pi = new predicate_item_true(neg, once, call, num, cl, this);
				}
				else if (iid == "open") {
					pi = new predicate_item_open(neg, once, call, num, cl, this);
				}
				else if (iid == "close") {
					pi = new predicate_item_close(neg, once, call, num, cl, this);
				}
				else if (iid == "get_char") {
					pi = new predicate_item_get_or_peek_char(false, neg, once, call, num, cl, this);
				}
				else if (iid == "peek_char") {
					pi = new predicate_item_get_or_peek_char(true, neg, once, call, num, cl, this);
				}
				else if (iid == "read_token") {
					pi = new predicate_item_read_token(neg, once, call, num, cl, this);
				}
				else if (iid == "write") {
					pi = new predicate_item_write(neg, once, call, num, cl, this);
				}
				else if (iid == "nl") {
					pi = new predicate_item_nl(neg, once, call, num, cl, this);
				}
				else if (iid == "seeing") {
					pi = new predicate_item_seeing(neg, once, call, num, cl, this);
				}
				else if (iid == "telling") {
					pi = new predicate_item_telling(neg, once, call, num, cl, this);
				}
				else if (iid == "seen") {
					pi = new predicate_item_seen(neg, once, call, num, cl, this);
				}
				else if (iid == "told") {
					pi = new predicate_item_told(neg, once, call, num, cl, this);
				}
				else if (iid == "see") {
					pi = new predicate_item_see(neg, once, call, num, cl, this);
				}
				else if (iid == "tell") {
					pi = new predicate_item_tell(neg, once, call, num, cl, this);
				}
				else if (iid == "open_url") {
					pi = new predicate_item_open_url(neg, once, call, num, cl, this);
				}
				else if (iid == "track_post") {
					pi = new predicate_item_track_post(neg, once, call, num, cl, this);
				}
				else if (iid == "consult") {
					pi = new predicate_item_consult(neg, once, call, num, cl, this);
				}
				else if (iid == "assert" || iid == "asserta" || iid == "assertz") {
					pi = new predicate_item_assert(iid != "asserta", neg, once, call, num, cl, this);
				}
				else if (iid == "retract") {
					pi = new predicate_item_retract(false, neg, once, call, num, cl, this);
				}
				else if (iid == "retractall") {
					pi = new predicate_item_retract(true, neg, once, call, num, cl, this);
				}
				else if (iid == "inc") {
					pi = new predicate_item_inc(neg, once, call, num, cl, this);
				}
				else if (iid == "halt") {
					exit(0);
				}
				else {
					pi = new predicate_item_user(neg, once, call, num, cl, this, iid);
				}

				if (p < s.length() && s[p] == '(') {
					p++;
					while (p < s.length() && s[p] != ')') {
						int oldp = p;
						value * dummy = parse(true, true, ff, s, p);
						if (dummy) {
							dummy->free();
							pi->add_arg(s.substr(oldp, p - oldp));
						}
						else {
							std::cout << id << ":" << iid << " : strange call head!" << endl;
							exit(2);
						}
						if (!bypass_spaces(s, p)) {
							std::cout << id << ":" << iid << " : strange call head!" << endl;
							exit(2);
						}
						if (s[p] == ',') p++;
						else if (s[p] != ')') {
							std::cout << id << ":" << iid << " : unexpected '" << s[p] << "' in call head!" << endl;
							exit(2);
						}
					}
					p++;
				}
			}

			if (pi) {
				if (or_branch) {
					int n = 0;
					while (!brackets.empty() && !brackets.top()) {
						brackets.pop();
						n++;
					}
					if (brackets.empty()) {
						std::cout << "Can't understand logical structure in " << id << " clause [" << s.substr(p - 10, 100) << "] : probably 'once' tangled?" << endl;
						exit(2);
					}
					brackets.top()->push_branch(pi);
					while (n--) brackets.push(NULL);
					or_branch = false;
				}
				cl->add_item(pi);
				num++;
				prev_pi = pi;
			}

			bool open_bracket = dynamic_cast<predicate_left_bracket *>(pi) != NULL;

			bypass_spaces(s, p);
			if (p < s.length()) {
				if (!open_bracket && (s[p] == ',' && (or_levels.size() == 0 || or_levels.top() != brackets.size()))) p++;
				else if (!open_bracket && (s[p] == ';' || s[p] == ',' && or_levels.size() > 0 && or_levels.top() == brackets.size())) {
					p++;
					if (brackets.size() == 0) {
						clause * _cl = new clause(pr);
						pr->add_clause(_cl);
						_cl->set_args(*cl->get_args());
						num = 0;
						cl = _cl;
					}
					else {
						predicate_or * _pi = new predicate_or(num, cl, this);
						ors.top().push(_pi);
						cl->add_item(_pi);
						num++;
						or_branch = true;
					}
				}
				else if (p + 1 < s.length() && s[p] == '-' && s[p + 1] == '>') {
					p += 2;
					if (pi == NULL) pi = prev_pi;
					if (dynamic_cast<predicate_right_bracket *>(pi))
						pi = ((predicate_right_bracket *)pi)->get_corresponding();
					if (pi) {
						predicate_item_cut * frontier = new predicate_item_cut(pi, num, cl, this);
						cl->add_item(frontier);
						num++;
						prev_pi = frontier;
					}
					else {
						std::cout << "Can't understand -> in " << id << " clause [" << s.substr(p-10, 100) << "]!" << endl;
						exit(2);
					}
				}
				else if (s[p] == '.') {
					p++;
					s.insert(s.begin() + p, ')');
					p++;

					if (!brackets.top()) {
						std::cout << "Brackets not balanced or problems with ';'-branches in " << id << " clause!" << endl;
						exit(2);
					}
					else {
						brackets.top()->clear_negated();

						if (or_levels.size() > 0 && or_levels.top() == brackets.size()) {
							or_levels.pop();
						}

						pi = new predicate_right_bracket(brackets.top(), num, cl, this);
						if (or_branch) {
							brackets.top()->push_branch(pi);
							or_branch = false;
						}
						brackets.pop();
						stack<predicate_or *> & or_items = ors.top();
						while (!or_items.empty()) {
							or_items.top()->set_ending(pi);
							or_items.pop();
						}
						ors.pop();

						cl->add_item(pi);
						num++;
					}

					if (or_branch || brackets.size()) {
						std::cout << "Brackets not balanced or problems with ';'-branches in " << id << " clause!" << endl;
						exit(2);
					}
					break;
				}
			}
			else {
				std::cout << "'(' or ')' or '.' or ',' or ';' expected in " << id << " clause!" << endl;
				exit(2);
			}
		} while (true);
	}
}

void interpreter::parse_program(vector<string> & renew, string & s) {
	int p = 0;
	do {
		bypass_spaces(s, p);
		if (p < s.length()) {
			frame_item * dummy = new frame_item();
			parse_clause(renew, dummy, s, p);
			delete dummy;
		}
		else
			break;
	} while (true);
}

void interpreter::add_std_body(string & body) {
	body.append("subtract([],_,[]).");
	body.append("\n");
	body.append("subtract([H|T],L1,L2):-once(member(H,L1)), once(subtract(T,L1,L2)).");
	body.append("\n");
	body.append("subtract([H|T],L1,[H|L2]):-\\+ member(H,L1), once(subtract(T,L1,L2)).");
	body.append("\n");
}

void interpreter::consult(const string & fname, bool renew) {
	string body;
	ifstream in(fname);

	if (in) {
		string line;
		add_std_body(body);
		while (getline(in, line)) {
			body += line;
			body += "\n";
		}
		in.close();
	}

	vector<string> _renew;
	if (renew) {
		map<string, predicate *>::iterator it = PREDICATES.begin();
		while (it != PREDICATES.end()) {
			_renew.push_back(it->first);
			it++;
		}
	}

	parse_program(_renew, body);
	bind();
}

void interpreter::bind() {
	map<string, predicate *>::iterator it = PREDICATES.begin();
	while (it != PREDICATES.end()) {
		it->second->bind();
		it++;
	}
}

predicate_item_user * interpreter::load_program(const string & fname, const string & starter_name) {
	consult(fname, false);

	predicate_item_user * result = PREDICATES[starter_name] ? new predicate_item_user(false, false, false, 0, NULL, this, starter_name) : NULL;

	if (result) result->bind();

	return result;
}

bool interpreter::loaded() {
	return starter != NULL;
}

void interpreter::run() {
	vector<value *> * args = new vector<value *>();
	generated_vars * variants = new generated_vars();
	frame_item * ff = new frame_item();
	variants->push_back(ff);
	starter->processing(false, 0, variants, args, ff);
	delete args;
	delete variants;
	delete ff;
}

interpreter::interpreter(const string & fname, const string & starter_name) {
	file_num = 0;
	current_input = STD_INPUT;
	current_output = STD_OUTPUT;
	ins = &cin;
	outs = &std::cout;

	CALLS.reserve(50000);
	FRAMES.reserve(50000);
	NEGS.reserve(50000);
	_FLAGS.reserve(50000);
	PARENT_CALLS.reserve(50000);
	PARENT_CALL_VARIANT.reserve(50000);
	CLAUSES.reserve(50000);
	// FLAGS;
	LEVELS.reserve(50000);
	TRACE.reserve(50000);
	TRACERS.reserve(50000);
	ptrTRACE.reserve(50000);

	if (fname.length() > 0)
		starter = load_program(fname, starter_name);
	else
		starter = NULL;
}

interpreter::~interpreter() {
	if (current_output != STD_OUTPUT) {
		dynamic_cast<basic_ofstream<char> *>(outs)->close();
		delete outs;
	}

	if (current_input != STD_INPUT) {
		dynamic_cast<basic_ifstream<char> *>(ins)->close();
		delete ins;
	}

	map<string, predicate *>::iterator it = PREDICATES.begin();
	while (it != PREDICATES.end()) {
		delete it->second;
		it++;
	}
	map< string, vector<term *> *>::iterator itd = DB.begin();
	while (itd != DB.end()) {
		if (itd->second) {
			for (int i = 0; i < itd->second->size(); i++)
				delete itd->second->at(i);
		}
		delete itd->second;
		itd++;
	}
	map< string, set<int> *>::iterator iti = DBIndicators.begin();
	while (iti != DBIndicators.end()) {
		delete iti->second;
		iti++;
	}
	map<string, value *>::iterator itg = GVars.begin();
	while (itg != GVars.end()) {
		delete itg->second;
		itg++;
	}
}

string interpreter::open_file(const string & fname, const string & mode) {
	string result = "#";
	char buf[65];
	result += __itoa(file_num, buf, 10);

	files[file_num] = basic_fstream<char>();
	if (mode == "read")
		files[file_num].open(fname, ios_base::in);
	else if (mode == "write")
		files[file_num].open(fname, ios_base::out | ios_base::trunc);
	else if (mode == "append")
		files[file_num].open(fname, ios_base::app);
	else {
		std::cout << "Unknown file [" << fname << "] open mode = [" << mode << "]" << endl;
		exit(504);
	}

	if (files[file_num])
		file_num++;
	else {
		std::cout << "File [" << fname << "] can't be opened with mode = [" << mode << "]" << endl;
		exit(505);
	}

	return result;
}

void interpreter::close_file(const string & obj) {
	int fn = -1;
	get_file(obj, fn).close();
	files.erase(fn);
}

basic_fstream<char> & interpreter::get_file(const string & obj, int & fn) {
	if (obj.length() < 2 || obj[0] != '#') {
		std::cout << "No such file: id = [" << obj << "]" << endl;
		exit(501);
	}
	fn = atoi(obj.substr(1).c_str());
	if (fn < 0 || fn >= file_num) {
		std::cout << "Incorrect file number = [" << fn << "]" << endl;
		exit(502);
	}

	map<int, basic_fstream<char> >::iterator it = files.find(fn);
	if (it == files.end()) {
		std::cout << "Probably file [" << fn << "] is already closed" << endl;
		exit(503);
	}

	return it->second;
}

bool interpreter::check_consistency(set<string> & dynamic_prefixes) {
	bool result = true;

	map<string, predicate *>::iterator pit = PREDICATES.begin();
	while (pit != PREDICATES.end()) {
		for (int c = 0; c < pit->second->num_clauses(); c++) {
			clause * cl = pit->second->get_clause(c);
			for (int i = 0; i < cl->num_items(); i++) {
				predicate_item_user * pp = dynamic_cast<predicate_item_user *>(cl->get_item(i));
				if (pp) {
					map<string, predicate *>::iterator ppit = PREDICATES.find(pp->get_id());
					if (ppit == PREDICATES.end() || pp->get_args()->size() != ppit->second->get_clause(0)->get_args()->size()) {
						set<string>::iterator pref_it = dynamic_prefixes.end();
						string ppid = pp->get_id();
						bool found = false;
						do {
							if (pref_it == dynamic_prefixes.begin()) break;
							pref_it--;
							if (ppid.substr(0, pref_it->length()) == *pref_it) {
								found = true;
							}
						} while (!found);
						if (!found) {
							(*outs) << "Predicate[" << pit->first << "][clause_" << c << "] : probably unknown predicate '" << ppid << "'/" << (pp->get_args()->size()) << endl;
							result = false;
						}
					}
				}
			}
		}
		pit++;
	}

	return result;
}

#ifdef __linux__
unsigned long long getTotalSystemMemory()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return (long long)pages * page_size;
}
#else
unsigned long long getTotalSystemMemory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
}
#endif

int main(int argc, char ** argv) {
#ifdef __linux__
	struct rlimit rl = { RLIM_INFINITY, RLIM_INFINITY };
	int result = setrlimit(RLIMIT_STACK, &rl);
	if (result != 0) {
		std::cout << "setrlimit returned result = " << result << endl;
		exit(1000);
	}
#endif

	fast_memory_manager = getTotalSystemMemory() > (long long)8 * (long long) 1024 * (long long) 1024 * (long long) 1024;

	int main_part = 1;
	while (main_part < argc) {
		string flag = string(argv[main_part]);
		if (flag == "--prefer-speed") fast_memory_manager = true;
		else if (flag == "--prefer-memory") fast_memory_manager = false;
		else break;
		main_part++;
	}

	if (argc == main_part) {
		interpreter prolog("", "");
		std::cout << "Prolog MicroBrain by V.V.Pekunov V0.21beta" << endl;
		std::cout << "  Enter 'halt.' or 'end_of_file' to exit." << endl << endl;
		while (true) {
			string line;
			int p = 0;

			std::cout << ">";
			getline(cin, line);

			if (line == "end_of_file")
				exit(0);

			vector<string> renew;
			frame_item * f = new frame_item();
			string body;

			map<string, predicate *>::iterator it = prolog.PREDICATES.begin();
			while (it != prolog.PREDICATES.end()) {
				renew.push_back(it->first);
				it++;
			}

			prolog.add_std_body(body);

			body.append("internal_goal:-");
			body.append(line);
			body.append("\n");
			prolog.bypass_spaces(body, p);
			while (p < body.length()) {
				prolog.parse_clause(renew, f, body, p);
				prolog.bypass_spaces(body, p);
			}
			prolog.bind();

			vector<value *> * args = new vector<value *>();
			generated_vars * variants = new generated_vars();
			variants->push_back(f);
			predicate_item_user * pi = new predicate_item_user(false, false, false, 0, NULL, &prolog, "internal_goal");
			pi->bind();
			bool ret = pi->processing(false, 0, variants, args, f);

			int itv = 0;
			while (itv < f->get_size()) {
				std::cout << "  " << f->atn(itv) << " = ";
				f->at(itv)->write(&std::cout);
				std::cout << endl;
				itv++;
			}

			delete args;
			delete variants;

			std::cout << endl;
			std::cout << (ret ? "true" : "false") << endl;
			
			delete pi;
			delete f;
		}
	}
	else if (argc - main_part == 3 && (string(argv[main_part]) == "-consult" || string(argv[main_part]) == "--consult")) {
		interpreter prolog(argv[main_part+1], argv[main_part+2]);

		if (prolog.loaded()) {
			prolog.run();
		}
		else
			std::cout << "Goal absent!" << endl;
	}
	else {
		std::cout << "Usage: prolog_micro_brain.exe [FLAGS] [-consult <file.pro> <goal_predicate_name>]" << endl;
		std::cout << "   FLAGS:" << endl;
		std::cout << "      --prefer-speed    -- Accelerates program but may consume a lot of memory" << endl;
		std::cout << "      --prefer-memory   -- Uses a minimal amount of memory but is slower" << endl;
	}
	return 0;
}