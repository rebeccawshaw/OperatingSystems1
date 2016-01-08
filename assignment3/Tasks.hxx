#ifndef CONSUMERS_HXX
#define CONSUMERS_HXX

#include "Task.hxx"
#include "Queue.hxx"
#include <map>
class Generator :
  public virtual Task < long long >
{
public:
  Generator (SQueue <long long> &dest, long long upper_bound);

  virtual long long svc (void) override final;

private:
  SQueue<long long> &dest_;
  long long u_bound_;
};

class PrimeTester :
  public virtual Task < long long >
{
public:
  PrimeTester (SQueue<long long> &source, SQueue<long long> &factorizer, SQueue<std::string> &output);

  virtual long long svc (void) override final;

  size_t primes_found (void);
private:
  /*
   * Don't worry about this stuff down here.  I got it from a helpful
   * stack exchange post on checking for primeness.
   */
  long long power(int a, int n, int mod);
  bool witness(int a, int n);
  bool is_prime ( int number );
  SQueue<long long> &source_, &fact_;
  SQueue<std::string> &dest_;
  size_t found_;
};


class Factorizer :
  public virtual Task < long long >
{
public:
  Factorizer (SQueue<long long> &source, SQueue<std::string> &dest);

  virtual long long svc (void) override final;

  typedef std::map <long long, size_t> factors;

private:
  void prime_factors (long long number, factors &);
  SQueue <long long> &source_;
  SQueue <std::string> &dest_;
};

#endif
