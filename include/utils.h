#ifndef ___UTILS_H_
#define ___UTILS_H_

#ifdef _WIN32
	#define strdup my_strdup
	#define strcasestr my_strcasestr
	#define strcasecmp my_strcasecmp
	#define strncasecmp my_strncasecmp
	#ifdef DLL_EXPORTS
		#define DLL_DECLSPEC __declspec(dllexport)
	#endif
	#ifndef DLL_EXPORTS
		#define DLL_DECLSPEC __declspec(dllimport)
	#endif
#endif

#ifndef _WIN32
	#define DLL_DECLSPEC
#endif

DLL_DECLSPEC void
trim_whitespace (char *);

DLL_DECLSPEC int
cmp_msg_file (char *m1, char *m2);

DLL_DECLSPEC char *
my_strcasestr(const char *s, const char *find);

DLL_DECLSPEC int
my_strcasecmp(const char *s1, const char *s2);

DLL_DECLSPEC int
my_strncasecmp(const char *s1, const char *s2, int n);

DLL_DECLSPEC char *
my_strdup(const char *s);


#endif
