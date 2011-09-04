/*** builtin-declarations.h - generated by builtin-maker */
#include "builtin-t.h"

/** aio.c */

/** aliases.c */

/** alloca.c */
void * __builtin_alloca (size_t size);

/** argp.c */

/** argz.c */

/** arpa/inet.c */

/** arpa/nameser.c */

/** assert.c */

/** bits/byteswap.c */

/** bits/dlfcn.c */

/** bits/errno.c */

/** bits/libc-lock.c */

/** bits/sched.c */

/** bits/select.c */

/** bits/shm.c */

/** bits/siginfo.c */

/** bits/sigset.c */

/** bits/sigthread.c */

/** bits/socket.c */

/** bits/string2.c */

/** bits/string.c */

/** bits/stropts.c */

/** bits/sys_errlist.c */

/** bits/time.c */

/** complex.c */
double complex __builtin_cacos (double complex z);
double complex __builtin_casin (double complex z);
double complex __builtin_catan (double complex z);
double complex __builtin_ccos (double complex z);
double complex __builtin_csin (double complex z);
double complex __builtin_ctan (double complex z);
double complex __builtin_cacosh (double complex z);
double complex __builtin_casinh (double complex z);
double complex __builtin_catanh (double complex z);
double complex __builtin_ccosh (double complex z);
double complex __builtin_csinh (double complex z);
double complex __builtin_ctanh (double complex z);
double complex __builtin_cexp (double complex z);
double complex __builtin_clog (double complex z);
double complex __builtin_cpow (double complex __x, double complex __y);
double complex __builtin_csqrt (double complex z);
double __builtin_cabs (double complex z);
double __builtin_carg (double complex z);
double complex __builtin_conj (double complex z);
double complex __builtin_cproj (double complex z);
double __builtin_cimag (double complex z);
double __builtin_creal (double complex z);
float complex __builtin_cacosf (float complex z);
float complex __builtin_casinf (float complex z);
float complex __builtin_catanf (float complex z);
float complex __builtin_ccosf (float complex z);
float complex __builtin_csinf (float complex z);
float complex __builtin_ctanf (float complex z);
float complex __builtin_cacoshf (float complex z);
float complex __builtin_casinhf (float complex z);
float complex __builtin_catanhf (float complex z);
float complex __builtin_ccoshf (float complex z);
float complex __builtin_csinhf (float complex z);
float complex __builtin_ctanhf (float complex z);
float complex __builtin_cexpf (float complex z);
float complex __builtin_clogf (float complex z);
float complex __builtin_cpowf (float complex __x, float complex __y);
float complex __builtin_csqrtf (float complex z);
float __builtin_cabsf (float complex z);
float __builtin_cargf (float complex z);
float complex __builtin_conjf (float complex z);
float complex __builtin_cprojf (float complex z);
float __builtin_cimagf (float complex z);
float __builtin_crealf (float complex z);
long double complex __builtin_cacosl (long double complex z);
long double complex __builtin_casinl (long double complex z);
long double complex __builtin_catanl (long double complex z);
long double complex __builtin_ccosl (long double complex z);
long double complex __builtin_csinl (long double complex z);
long double complex __builtin_ctanl (long double complex z);
long double complex __builtin_cacoshl (long double complex z);
long double complex __builtin_casinhl (long double complex z);
long double complex __builtin_catanhl (long double complex z);
long double complex __builtin_ccoshl (long double complex z);
long double complex __builtin_csinhl (long double complex z);
long double complex __builtin_ctanhl (long double complex z);
long double complex __builtin_cexpl (long double complex z);
long double complex __builtin_clogl (long double complex z);
long double complex __builtin_cpowl (long double complex __x, long double complex __y);
long double complex __builtin_csqrtl (long double complex z);
long double __builtin_cabsl (long double complex z);
long double __builtin_cargl (long double complex z);
long double complex __builtin_conjl (long double complex z);
long double complex __builtin_cprojl (long double complex z);
long double __builtin_cimagl (long double complex z);
long double __builtin_creall (long double complex z);

/** crypt.c */

/** ctype.c */

/** dirent.c */

/** dlfcn.c */

/** envz.c */

/** err.c */

/** error.c */

/** execinfo.c */

/** fcntl.c */

/** fenv.c */

/** fmtmsg.c */

/** fnmatch.c */

/** fstab.c */

/** fts.c */

/** ftw.c */

/** getopt.c */

/** glob.c */

/** gnu/libc-version.c */

/** grp.c */

/** iconv.c */

/** ifaddrs.c */

/** inttypes.c */

/** langinfo.c */

/** libgen.c */

/** libintl.c */
char * __builtin_gettext (const char *msgid);
char * __builtin_dgettext (const char *domainname, const char *msgid);
char * __builtin_dcgettext (const char *domainname, __const char *msgid, int category);

/** link.c */

/** linux/kernel.c */

/** locale.c */

/** _main.c */

/** malloc.c */
__malloc_ptr_t __builtin_malloc (size_t size);
__malloc_ptr_t __builtin_calloc (size_t nmemb, size_t size);

/** math.c */
double __builtin_acos (double x);
double __builtin_asin (double x);
double __builtin_atan (double x);
double __builtin_atan2 (double y, double x);
double __builtin_cos (double x);
double __builtin_sin (double x);
double __builtin_tan (double x);
double __builtin_cosh (double x);
double __builtin_sinh (double x);
double __builtin_tanh (double x);
void __builtin_sincos (double x, double *sinx, double *cosx);
double __builtin_acosh (double x);
double __builtin_asinh (double x);
double __builtin_atanh (double x);
double __builtin_exp (double x);
double __builtin_frexp (double x, int *exponent);
double __builtin_ldexp (double x, int exponent);
double __builtin_log (double x);
double __builtin_log10 (double x);
double __builtin_modf (double x, double *iptr);
double __builtin_exp10 (double x);
double __builtin_pow10 (double x);
double __builtin_expm1 (double x);
double __builtin_log1p (double x);
double __builtin_logb (double x);
double __builtin_exp2 (double x);
double __builtin_log2 (double x);
double __builtin_pow (double x, double y);
double __builtin_sqrt (double x);
double __builtin_hypot (double x, double y);
double __builtin_cbrt (double x);
double __builtin_ceil (double x);
double __builtin_fabs (double x);
double __builtin_floor (double x);
double __builtin_fmod (double x, double y);
double __builtin_drem (double x, double y);
double __builtin_significand (double x);
double __builtin_copysign (double x, double y);
double __builtin_nan (const char *tagb);
double __builtin_j0 (double x);
double __builtin_j1 (double x);
double __builtin_jn (int n, double x);
double __builtin_y0 (double x);
double __builtin_y1 (double x);
double __builtin_yn (int n, double x);
double __builtin_erf (double x);
double __builtin_erfc (double x);
double __builtin_lgamma (double x);
double __builtin_tgamma (double x);
double __builtin_gamma (double x);
double __builtin_rint (double x);
double __builtin_nextafter (double x, double y);
double __builtin_nexttoward (double x, long double y);
double __builtin_remainder (double x, double y);
double __builtin_scalbn (double x, int n);
int __builtin_ilogb (double x);
double __builtin_scalbln (double x, long int n);
double __builtin_nearbyint (double x);
double __builtin_round (double x);
double __builtin_trunc (double x);
double __builtin_remquo (double x, double y, int *quo);
long int __builtin_lrint (double x);
long long int __builtin_llrint (double x);
long int __builtin_lround (double x);
long long int __builtin_llround (double x);
double __builtin_fdim (double x, double y);
double __builtin_fmax (double x, double y);
double __builtin_fmin (double x, double y);
double __builtin_fma (double x, double y, double z);
double __builtin_scalb (double x, double n);
float __builtin_acosf (float x);
float __builtin_asinf (float x);
float __builtin_atanf (float x);
float __builtin_atan2f (float y, float x);
float __builtin_cosf (float x);
float __builtin_sinf (float x);
float __builtin_tanf (float x);
float __builtin_coshf (float x);
float __builtin_sinhf (float x);
float __builtin_tanhf (float x);
void __builtin_sincosf (float x, float *sinx, float *cosx);
float __builtin_acoshf (float x);
float __builtin_asinhf (float x);
float __builtin_atanhf (float x);
float __builtin_expf (float x);
float __builtin_frexpf (float x, int *exponent);
float __builtin_ldexpf (float x, int exponent);
float __builtin_logf (float x);
float __builtin_log10f (float x);
float __builtin_modff (float x, float *iptr);
float __builtin_exp10f (float x);
float __builtin_pow10f (float x);
float __builtin_expm1f (float x);
float __builtin_log1pf (float x);
float __builtin_logbf (float x);
float __builtin_exp2f (float x);
float __builtin_log2f (float x);
float __builtin_powf (float x, float y);
float __builtin_sqrtf (float x);
float __builtin_hypotf (float x, float y);
float __builtin_cbrtf (float x);
float __builtin_ceilf (float x);
float __builtin_fabsf (float x);
float __builtin_floorf (float x);
float __builtin_fmodf (float x, float y);
float __builtin_dremf (float x, float y);
float __builtin_significandf (float x);
float __builtin_copysignf (float x, float y);
float __builtin_nanf (const char *tagb);
float __builtin_j0f (float x);
float __builtin_j1f (float x);
float __builtin_jnf (int n, float x);
float __builtin_y0f (float x);
float __builtin_y1f (float x);
float __builtin_ynf (int n, float x);
float __builtin_erff (float x);
float __builtin_erfcf (float x);
float __builtin_lgammaf (float x);
float __builtin_tgammaf (float x);
float __builtin_gammaf (float x);
float __builtin_rintf (float x);
float __builtin_nextafterf (float x, float y);
float __builtin_nexttowardf (float x, long double y);
float __builtin_remainderf (float x, float y);
float __builtin_scalbnf (float x, int n);
int __builtin_ilogbf (float x);
float __builtin_scalblnf (float x, long int n);
float __builtin_nearbyintf (float x);
float __builtin_roundf (float x);
float __builtin_truncf (float x);
float __builtin_remquof (float x, float y, int *quo);
long int __builtin_lrintf (float x);
long long int __builtin_llrintf (float x);
long int __builtin_lroundf (float x);
long long int __builtin_llroundf (float x);
float __builtin_fdimf (float x, float y);
float __builtin_fmaxf (float x, float y);
float __builtin_fminf (float x, float y);
float __builtin_fmaf (float x, float y, float z);
float __builtin_scalbf (float x, float n);
long double __builtin_acosl (long double x);
long double __builtin_asinl (long double x);
long double __builtin_atanl (long double x);
long double __builtin_atan2l (long double y, long double x);
long double __builtin_cosl (long double x);
long double __builtin_sinl (long double x);
long double __builtin_tanl (long double x);
long double __builtin_coshl (long double x);
long double __builtin_sinhl (long double x);
long double __builtin_tanhl (long double x);
void __builtin_sincosl (long double x, long double *sinx, long double *cosx);
long double __builtin_acoshl (long double x);
long double __builtin_asinhl (long double x);
long double __builtin_atanhl (long double x);
long double __builtin_expl (long double x);
long double __builtin_frexpl (long double x, int *exponent);
long double __builtin_ldexpl (long double x, int exponent);
long double __builtin_logl (long double x);
long double __builtin_log10l (long double x);
long double __builtin_modfl (long double x, long double *iptr);
long double __builtin_exp10l (long double x);
long double __builtin_pow10l (long double x);
long double __builtin_expm1l (long double x);
long double __builtin_log1pl (long double x);
long double __builtin_logbl (long double x);
long double __builtin_exp2l (long double x);
long double __builtin_log2l (long double x);
long double __builtin_powl (long double x, long double y);
long double __builtin_sqrtl (long double x);
long double __builtin_hypotl (long double x, long double y);
long double __builtin_cbrtl (long double x);
long double __builtin_ceill (long double x);
long double __builtin_fabsl (long double x);
long double __builtin_floorl (long double x);
long double __builtin_fmodl (long double x, long double y);
long double __builtin_dreml (long double x, long double y);
long double __builtin_significandl (long double x);
long double __builtin_copysignl (long double x, long double y);
long double __builtin_nanl (const char *tagb);
long double __builtin_j0l (long double x);
long double __builtin_j1l (long double x);
long double __builtin_jnl (int n, long double x);
long double __builtin_y0l (long double x);
long double __builtin_y1l (long double x);
long double __builtin_ynl (int n, long double x);
long double __builtin_erfl (long double x);
long double __builtin_erfcl (long double x);
long double __builtin_lgammal (long double x);
long double __builtin_tgammal (long double x);
long double __builtin_gammal (long double x);
long double __builtin_rintl (long double x);
long double __builtin_nextafterl (long double x, long double y);
long double __builtin_nexttowardl (long double x, long double y);
long double __builtin_remainderl (long double x, long double y);
long double __builtin_scalbnl (long double x, int n);
int __builtin_ilogbl (long double x);
long double __builtin_scalblnl (long double x, long int n);
long double __builtin_nearbyintl (long double x);
long double __builtin_roundl (long double x);
long double __builtin_truncl (long double x);
long double __builtin_remquol (long double x, long double y, int *quo);
long int __builtin_lrintl (long double x);
long long int __builtin_llrintl (long double x);
long int __builtin_lroundl (long double x);
long long int __builtin_llroundl (long double x);
long double __builtin_fdiml (long double x, long double y);
long double __builtin_fmaxl (long double x, long double y);
long double __builtin_fminl (long double x, long double y);
long double __builtin_fmal (long double x, long double y, long double z);
long double __builtin_scalbl (long double x, long double n);
double __builtin_huge_val (void);
float __builtin_huge_valf (void);
long double __builtin_huge_vall (void);
double __builtin_inf (void);
float __builtin_inff (void);
long double __builtin_infl (void);
double __builtin_nans (const char *str);
float __builtin_nansf (const char *str);
long double __builtin_nansl (const char *str);

/** mcheck.c */

/** mntent.c */

/** monetary.c */
ssize_t __builtin_strfmon (char * s, size_t maxsize, const char * format, ...);

/** netdb.c */

/** net/if.c */

/** netinet/ether.c */

/** netinet/in.c */

/** nl_types.c */

/** nss.c */

/** obstack.c */

/** printf.c */

/** pthread.c */

/** pty.c */

/** pwd.c */

/** regex.c */

/** regexp.c */

/** resolv.c */

/** rpc/auth.c */

/** rpc/auth_des.c */

/** rpc/auth_unix.c */

/** rpc/clnt.c */

/** rpc/des_crypt.c */

/** rpc/key_prot.c */

/** rpc/netdb.c */

/** rpc/pmap_clnt.c */

/** rpc/pmap_prot.c */

/** rpc/pmap_rmt.c */

/** rpc/rpc.c */

/** rpc/rpc_msg.c */

/** rpc/svc_auth.c */

/** rpcsvc/bootparam_prot.c */

/** rpc/svc.c */

/** rpcsvc/key_prot.c */

/** rpcsvc/klm_prot.c */

/** rpcsvc/mount.c */

/** rpcsvc/nfs_prot.c */

/** rpcsvc/nis.c */

/** rpcsvc/nis_callback.c */

/** rpcsvc/nislib.c */

/** rpcsvc/nlm_prot.c */

/** rpcsvc/rex.c */

/** rpcsvc/rquota.c */

/** rpcsvc/rstat.c */

/** rpcsvc/rusers.c */

/** rpcsvc/sm_inter.c */

/** rpcsvc/spray.c */

/** rpcsvc/yp.c */

/** rpcsvc/ypclnt.c */

/** rpcsvc/yppasswd.c */

/** rpcsvc/yp_prot.c */

/** rpcsvc/ypupd.c */

/** rpc/xdr.c */

/** sched.c */

/** search.c */

/** semaphore.c */

/** setjmp.c */

/** sgtty.c */

/** shadow.c */

/** signal.c */

/** spawn.c */

/** stdio.c */
int __builtin_fprintf (FILE * stream, const char * format, ...);
int __builtin_fprintf_unlocked (FILE * stream, const char * format, ...);
int __builtin_printf (const char * format, ...);
int __builtin_printf_unlocked (const char * format, ...);
int __builtin_sprintf (char * s, const char * format, ...);
int __builtin_vfprintf (FILE * s, const char * format, va_list arg);
int __builtin_vprintf (const char * format, va_list arg);
int __builtin_vsprintf (char * s, const char * format, va_list arg);
int __builtin_snprintf (char * s, size_t maxlen, const char * format, ...);
int __builtin_vsnprintf (char * s, size_t maxlen, const char * format, va_list arg);
int __builtin_fscanf (FILE * stream, const char * format, ...);
int __builtin_scanf (const char * format, ...);
int __builtin_sscanf (const char * s, const char * format, ...);
int __builtin_vfscanf (FILE * s, const char * format, va_list arg);
int __builtin_vscanf (const char * format, va_list arg);
int __builtin_vsscanf (const char * s, const char * format, va_list arg);
int __builtin_fputc (int c, FILE *stream);
int __builtin_putchar (int c);
int __builtin_putchar_unlocked (int c);
int __builtin_fputs (const char * s, FILE * stream);
int __builtin_puts (const char *__s);
int __builtin_puts_unlocked (const char *__s);
size_t __builtin_fwrite (const void * ptr, size_t size, size_t n, FILE * s);
int __builtin_fputs_unlocked (const char * s, FILE * stream);
size_t __builtin_fwrite_unlocked (const void * ptr, size_t size, size_t n, FILE * stream);

/** stdio_ext.c */

/** stdlib.c */
void __builtin_abort (void);
void __builtin_exit (int status);
void __builtin__Exit (int status);
int __builtin_abs (int x);
long int __builtin_labs (long int x);
long long int __builtin_llabs (long long int x);

/** string.c */
void * __builtin_memcpy (void * dest, void const * src, size_t n);
void * __builtin_memmove (void * dest, const void * src, size_t n);
void * __builtin_memset (void * s, int c, size_t n);
int __builtin_memcmp (const void * s1, const void * s2, size_t n);
char * __builtin_strcpy (char * dest, const char * src);
char * __builtin_strncpy (char * dest, const char * src, size_t n);
char * __builtin_strcat (char * dest, const char * src);
char * __builtin_strncat (char * dest, const char * src, size_t n);
int __builtin_strcmp (const char * s1, const char * s2);
int __builtin_strncmp (const char * s1, const char * s2, size_t n);
char * __builtin_strdup (const char * s1);
char * __builtin_strchr (const char * s, int c);
char * __builtin_strrchr (const char * s, int c);
size_t __builtin_strcspn (const char * s, const char * reject);
size_t __builtin_strspn (const char * s, const char * accept);
char * __builtin_strpbrk (const char * s, const char * accept);
char * __builtin_strstr (const char * haystack, const char * needle);
void * __builtin_mempcpy (void * dest, const void * src, size_t n);
size_t __builtin_strlen (const char * s);
void __builtin_bzero (void * s, size_t n);
void __builtin_bcopy (const void * src, void * dest, size_t n);
int __builtin_bcmp (const void * s1, const void * s2, size_t n);
char * __builtin_index (const char * s, int c);
char * __builtin_rindex (const char * s, int c);
int __builtin_ffs (int i);
int __builtin_ffsl (long int l);
int __builtin_ffsll (long long int ll);
char * __builtin_stpcpy (char * dest, const char * src);

/** stropts.c */

/** sys/acct.c */

/** sys/epoll.c */

/** sys/file.c */

/** sys/fsuid.c */

/** sys/gmon.c */

/** sys/io.c */

/** sys/ioctl.c */

/** sys/ipc.c */

/** sys/kdaemon.c */

/** sys/klog.c */

/** sys/mman.c */

/** sys/mount.c */

/** sys/msg.c */

/** sys/perm.c */

/** sys/personality.c */

/** sys/poll.c */

/** sys/prctl.c */

/** sys/profil.c */

/** sys/ptrace.c */

/** sys/quota.c */

/** sys/reboot.c */

/** sys/resource.c */

/** sys/select.c */

/** sys/sem.c */

/** sys/sendfile.c */

/** sys/shm.c */

/** sys/socket.c */

/** sys/stat.c */

/** sys/statfs.c */

/** sys/statvfs.c */

/** sys/swap.c */

/** sys/sysctl.c */

/** sys/sysinfo.c */

/** sys/syslog.c */

/** sys/sysmacros.c */

/** sys/timeb.c */

/** sys/time.c */

/** sys/times.c */

/** sys/timex.c */

/** sys/uio.c */

/** sys/ustat.c */

/** sys/utsname.c */

/** sys/vlimit.c */

/** sys/vm86.c */

/** sys/vtimes.c */

/** sys/wait.c */

/** sys/xattr.c */

/** termios.c */

/** thread_db.c */

/** time.c */
size_t __builtin_strftime (char * s, size_t maxsize, const char * format, const struct tm * tp);

/** ttyent.c */

/** ucontext.c */

/** ulimit.c */

/** unistd.c */

/** utime.c */

/** utmp.c */

/** utmpx.c */

/** wchar.c */

/** wctype.c */

/** wordexp.c */
