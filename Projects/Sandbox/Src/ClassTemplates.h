#pragma once

#define RS_NO_COPY(name) \
	name(const name&) = delete;	\
	name& operator=(const name&) = delete;

#define RS_NO_MOVE(name) \
	name(const name&&) = delete;	\
	name& operator=(const name&&) = delete;

#define RS_DEFAULT_COPY(name) \
	name(const name&) = default;	\
	name& operator=(const name&) = default;

#define RS_DEFAULT_CLASS(name) \
	name() = default; \
	~name() = default;

#define RS_VIRTUAL_DEFAULT_CLASS(name) \
	name() = default; \
	virtual ~name() = default;

#define RS_NO_COPY_AND_MOVE(name) \
	RS_NO_COPY(name) \
	RS_NO_MOVE(name)

#define RS_DEFAULT_ABSTRACT_CLASS(name) \
	RS_NO_COPY_AND_MOVE(name) \
	RS_VIRTUAL_DEFAULT_CLASS(name)

#define RS_STATIC_CLASS(name) \
	RS_NO_COPY_AND_MOVE(name) \
	RS_DEFAULT_CLASS(name)
/*
* This will remove all copy and move constructors and assignemnt operators.
* Furthermore, it will create a default destructor.
* To get a singleton class, a private constructor need to be decleared.
*/
#define RS_DEFAULT_SINGLETON(name) \
	RS_NO_COPY(name) \
	RS_NO_MOVE(name) \
	~name() = default;

#define RS_SINGLETON(name) \
	RS_NO_COPY(name) \
	RS_NO_MOVE(name) \
	~name();

#define RS_DEFAULT_CONSTRUCTORS(name) \
	RS_DEFAULT_COPY(name) \
	name() = default; \
	~name() = default;

#define RS_CONSTRUCTORS(name) \
	RS_DEFAULT_COPY(name) \
	name(); \
	~name();