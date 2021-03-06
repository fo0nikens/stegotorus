/* Copyright 2011 Nick Mathewson, George Kadianakis
 * Copyright 2011, 2012, 2013 SRI International
 * See LICENSE for other credits and copying information
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "cpp.h"
#include "util.h"
#include "stegerrors.h"

struct proto_module;
class steg_config_t;
class modus_operandi_t;

//#define RECV_GOOD 0
//#define RECV_INCOMPLETE 1
//#define RECV_BAD -1





/** A 'config_t' is a set of addresses to listen on, and what to do
    when connections are received.  A protocol module must define a
    private subclass of this type that implements all the methods
    below, plus a descendant constructor.  The subclass must have the
    name MODULE_config_t where MODULE is the module name you use in
    PROTO_DEFINE_MODULE.  Use CONFIG_DECLARE_METHODS in the
    declaration. */

class config_t
{

 public:

  struct event_base         *base;
  enum listen_mode           mode;
  /* stopgap, see create_outbound_connections_socks */
  bool ignore_socks_destination : 1;
  /* experimental trac ticket # 149 */
  const char *shared_secret;
  /* if we want apache out the front of the server, then a hostname is a good idea */
  const char *hostname;
  bool persist_mode;
  modus_operandi_t *mop;
  
 config_t() : 
  base(0), mode((enum listen_mode)-1), ignore_socks_destination(false), 
    shared_secret(NULL),  hostname(NULL), persist_mode(false), mop(NULL) {};

  virtual ~config_t();

  DISALLOW_COPY_AND_ASSIGN(config_t);

  /** Return the name of the protocol associated with this
      configuration.  You do not have to define this method in your
      subclass, PROTO_DEFINE_MODULE does it for you. */
  virtual const char *name() const = 0;

  /** Initialize yourself from a set of command line options.  This is
      separate from the subclass constructor so that it can fail:
      if the command line options are ill-formed, print a diagnostic
      on stderr and return false.  On success, return true. */
  virtual bool init(int n_opts, const char *const *opts, modus_operandi_t &mo) = 0;

  /** Helper function that isolates what the protocol requires of the 
       modus_operandi_t object. Or in other words what things must be defined
      in the configuraion file. */
  virtual bool is_good(modus_operandi_t &mo) = 0;

  /** Return a set of addresses to listen on, in the form of an
      'evutil_addrinfo' linked list.  There may be more than one list;
      users of this function should call it repeatedly with successive
      values of N, starting from zero, until it returns NULL, and
      create listeners for every address returned. */
  virtual evutil_addrinfo *get_listen_addrs(size_t n) const = 0;

  /** Return a set of addresses to attempt an outbound connection to,
      in the form of an 'evutil_addrinfo' linked list.  As with
      get_listen_addrs, there may be more than one such list; users
      should in general attempt simultaneous connection to at least
      one address from every list.  The maximum N is indicated in the
      same way as for get_listen_addrs.  */
  virtual evutil_addrinfo *get_target_addrs(size_t n) const = 0;

  /** Return the steganography module associated with either listener
      or target address set N.  If called on a protocol that doesn't
      use steganography, will return NULL.  */
  virtual const steg_config_t *get_steg(size_t n) const = 0;

  /** Return an extended 'circuit_t' object for a new socket using
      this configuration.  The 'index' argument is equal to the 'N'
      argument to get_listen_addrs or get_target_addrs that retrieved
      the address to which the socket is bound.  */
  virtual circuit_t *circuit_create(size_t index) = 0;

  /** Return an extended 'conn_t' object for a new socket using this
      configuration.  The 'index' argument is equal to the 'N'
      argument to get_listen_addrs or get_target_addrs that retrieved
      the address to which the socket is bound.  */

  virtual conn_t *conn_create(size_t index) = 0;

  virtual void socks_force_addr(const char* host, int port) = 0;


};

int config_is_supported(const char *name);
config_t *config_create(int n_options, const char *const *options, modus_operandi_t& mo);

/** PROTO_DEFINE_MODULE defines an object with this type, plus the
    function that it points to; there is a table of all such objects,
    which generic code uses to know what protocols are available. */
struct proto_module
{
  /** Name of this protocol. Must be a valid C identifier. */
  const char *name;

  /** Create a config_t instance for this module from a set of command
      line options. */
  config_t *(*config_create)(int n_options, const char *const *options, modus_operandi_t& mo);
};

extern const proto_module *const supported_protos[];

/** Use these macros to define protocol modules. */

#define PROTO_DEFINE_MODULE(mod)                                                      \
  /* canned methods */                                                                \
  const char *mod##_config_t::name() const                                            \
  { return #mod; }                                                                    \
                                                                                      \
  static config_t *                                                                   \
  mod##_config_create(int n_opts, const char *const *opts, modus_operandi_t& mo)      \
  { mod##_config_t *s = new mod##_config_t();                                         \
    if (s->init(n_opts, opts, mo))                                                    \
      return s;                                                                       \
    delete s;                                                                         \
    return 0;                                                                         \
  }                                                                                   \
                                                                                      \
  extern const proto_module p_mod_##mod = {                                           \
    #mod, mod##_config_create,                                                        \
  } /* deliberate absence of semicolon */

#define CONFIG_DECLARE_METHODS(mod)                                                   \
  mod##_config_t();                                                                   \
  virtual ~mod##_config_t();                                                          \
  virtual const char *name() const;                                                   \
  virtual bool init(int n_opts, const char *const *opts, modus_operandi_t& mo);       \
  virtual bool is_good(modus_operandi_t& mo);                                         \
  virtual evutil_addrinfo *get_listen_addrs(size_t n) const;                          \
  virtual evutil_addrinfo *get_target_addrs(size_t n) const;                          \
  virtual const steg_config_t *get_steg(size_t n) const;                              \
  virtual circuit_t *circuit_create(size_t index);                                    \
  virtual conn_t *conn_create(size_t index);				              \
  virtual void socks_force_addr(const char* host, int port)                           \
  /* deliberate absence of semicolon */

#define CONFIG_STEG_STUBS(mod)                                  \
  const steg_config_t *mod##_config_t::get_steg(size_t) const   \
  { return 0; }

#define CONN_DECLARE_METHODS(mod)                       \
  mod##_conn_t();                                       \
  virtual ~mod##_conn_t();                              \
  virtual void close();                                 \
  virtual circuit_t *circuit() const;                   \
  virtual int  maybe_open_upstream();                   \
  virtual int  handshake();                             \
  virtual int  recv();                                  \
  virtual int  recv_eof();                              \
  virtual void expect_close();                          \
  virtual void cease_transmission();                    \
  virtual void transmit_soon(unsigned long timeout)     \
  /* deliberate absence of semicolon */

#define CONN_STEG_STUBS(mod)                            \
  void mod##_conn_t::expect_close()                     \
  { log_abort(this, "steg stub called"); }              \
  void mod##_conn_t::cease_transmission()               \
  { log_abort(this, "steg stub called"); }              \
  void mod##_conn_t::transmit_soon(unsigned long)       \
  { log_abort(this, "steg stub called"); }

#define CIRCUIT_DECLARE_METHODS(mod)            \
  mod##_circuit_t();                            \
  virtual ~mod##_circuit_t();                   \
  virtual void close();                         \
  virtual config_t *cfg() const;                \
  virtual void add_downstream(conn_t *);        \
  virtual void drop_downstream(conn_t *);       \
  virtual int  send();                          \
  virtual int  send_eof();

#endif
