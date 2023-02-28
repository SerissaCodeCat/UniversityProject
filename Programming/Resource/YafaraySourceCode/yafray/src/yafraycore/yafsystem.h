#ifndef __YAFSYSTEM_H
#define __YAFSYSTEM_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#ifdef WIN32
#define NOMINMAX
#include <io.h>
#include <windows.h>
#endif

#include <list>
#include <string>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT sharedlibrary_t 
{
	public:
	  sharedlibrary_t();
	  sharedlibrary_t(const std::string &library);
	  sharedlibrary_t(const sharedlibrary_t &src);
	  ~sharedlibrary_t();

	  bool isOpen();
	  void* getSymbol(const char *name);

	protected:

	  void open(const std::string &library);
	  void close();
  	void addReference() { (*refcount)++; };
  	void removeReference() { (*refcount)--; };
	bool isUsed()const {return ((*refcount)>0);}; 


	int *refcount;
#ifdef WIN32
	HINSTANCE handle;
#else
	void *handle;
#endif
};

YAFRAYCORE_EXPORT const std::list<std::string> & listDir(const std::string &dir);


__END_YAFRAY

#endif
