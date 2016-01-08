#include "Tasks.hxx"
#include <sstream>

Generator::Generator (SQueue <long long> &dest, long long u_bound)
  : dest_ (dest),
    u_bound_ (u_bound)
{
}

long long
Generator::svc (void)
{
  std::mt19937_64 generator (std::chrono::system_clock::now ().time_since_epoch ().count () * pthread_self ());
  std::uniform_int_distribution<long long> uniform_gen (0, (u_bound_ == 0) ?
                                                        std::numeric_limits<long long>::max () : u_bound_);

  while (true)
    {
      long long candidate (generator ());
      this->dest_.enqueue (uniform_gen (generator));
    }
  return 0;
}

PrimeTester::PrimeTester (SQueue<long long> &source, SQueue<long long> &fact, SQueue<std::string> &dest)
  : source_ (source), fact_ (fact), dest_ (dest), found_ (0)
{
}

long long
PrimeTester::svc (void)
{
  while (true)
    {
      long long candidate = source_.dequeue ();

      if (is_prime (candidate))
        {
          ++found_;
          dest_.enqueue (std::to_string (candidate) + " is prime!\n");
        }
      else
        fact_.enqueue (candidate);
    }
  return 0;
}

size_t
PrimeTester::primes_found (void)
{
  return this->found_;
}

Factorizer::Factorizer (SQueue<long long> &source, SQueue<std::string> &dest)
  : source_ (source), dest_ (dest)
{
}

long long
Factorizer::svc (void)
{
  while (true)
    {
      long long candidate = source_.dequeue ();

      factors f;
      prime_factors (candidate, f);

      std::stringstream str;
      str << candidate << " is not prime, its factors are: ";

      std::for_each (f.begin (), f.end (), [&str] (const factors::value_type &item)
                     {
                       str << item.first << ", ";
                     });

      dest_.enqueue (str.str ());
    }
  return 0;
}

// ========================== Don't worry about any of this stuff down here!

long long
PrimeTester::power(int a, int n, int mod)
{
  long long power=a,result=1;

  while(n)
    {
      if(n&1)
        result=(result*power)%mod;
      power=(power*power)%mod;
      n>>=1;
    }

  return result;
}

bool
PrimeTester::witness(int a, int n)
{
  int t,u,i;
  long long prev,curr;

  u=n/2;
  t=1;
  while(!(u&1))
    {
      u/=2;
      ++t;
    }

  prev=power(a,u,n);
  for(i=1;i<=t;++i)
    {
      curr=(prev*prev)%n;
      if((curr==1)&&(prev!=1)&&(prev!=n-1))
        return true;
      prev=curr;
    }
  if(curr!=1)
    return true;
  return false;
}

bool
PrimeTester::is_prime ( int number )
{
  if ( ( (!(number & 1)) && number != 2 ) || (number < 2) || (number % 3 == 0 && number != 3) )
    return (false);

  if(number<1373653)
    {
      for( int k = 1; 36*k*k-12*k < number;++k)
        if ( (number % (6*k+1) == 0) || (number % (6*k-1) == 0) )
          return (false);

      return true;
    }

  if(number < 9080191)
    {
      if(witness(31,number)) return false;
      if(witness(73,number)) return false;
      return true;
    }


  if(witness(2,number)) return false;
  if(witness(7,number)) return false;
  if(witness(61,number)) return false;
  return true;

  /*WARNING: Algorithm deterministic only for numbers < 4,759,123,141 (unsigned int's max is 4294967296)
    if n < 1,373,653, it is enough to test a = 2 and 3.
    if n < 9,080,191, it is enough to test a = 31 and 73.
    if n < 4,759,123,141, it is enough to test a = 2, 7, and 61.
    if n < 2,152,302,898,747, it is enough to test a = 2, 3, 5, 7, and 11.
    if n < 3,474,749,660,383, it is enough to test a = 2, 3, 5, 7, 11, and 13.
    if n < 341,550,071,728,321, it is enough to test a = 2, 3, 5, 7, 11, 13, and 17.*/
}

// A function to print all prime factors of a given number n
void
Factorizer::prime_factors (long long n, factors &rv)
{
  size_t count(0);
  // Print the number of 2s that divide n
  while (n%2 == 0)
    {
      ++count;
      n = n/2;
    }

  if (count) rv[2] = count;
  count = 0;

  // n must be odd at this point.  So we can skip one element (Note i = i +2)
  for (int i = 3; i <= sqrt(n); i = i+2)
    {
      // While i divides n, print i and divide n
      while (n%i == 0)
        {
          ++count;
          n = n/i;
        }
      if (count) rv[i] = count;
      count = 0;
    }

  // This condition is to handle the case whien n is a prime number
  // greater than 2
  if (n > 2)
    rv[n] = 1;
}
