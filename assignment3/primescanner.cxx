#include <iostream>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "Queue.hxx"
#include "Tasks.hxx"

namespace po = boost::program_options;

int main (int argc, char **argv)
{
  try
    {
      po::options_description desc("Allowed options");
      desc.add_options ()
        ("help,h", "Produce this help message")
        ("num-generators,g", po::value<size_t> (),"Number of generator threads to spawn.")
        ("num-primetesters,p", po::value<size_t> (),"Number of prime tester threads to spawn.")
        ("num-factorizers,f", po::value<size_t> (),"Number of factorizer threads to spawn.")
        ("num-primes,n", po::value<size_t> (), "Stop after finding X primes.")
        ("upper-bound,u", po::value<long long> (), "Upper bound on values generated.")
        ;

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);

      if (vm.count ("help"))
        {
          std::cout << desc << "\n";
          return 0;
        }

      size_t n_generators (1), n_primetesters (1), n_factorizers (1), n_primes (100), u_bound (0);

      if (vm.count("num-generators")) n_generators = vm["num-generators"].as <size_t> ();
      if (vm.count("num-primetesters")) n_primetesters = vm["num-primetesters"].as <size_t> ();
      if (vm.count("num-factorizers")) n_factorizers = vm["num-factorizers"].as <size_t> ();
      if (vm.count("num-primes")) n_primes = vm["num-primes"].as <size_t> ();
      if (vm.count("upper-bound")) u_bound = vm["upper-bound"].as <long long> ();

      SQueue<long long> gen_to_pt(30), pt_to_fact(10);
      SQueue<std::string> output (5);

      Generator gen (gen_to_pt, u_bound);
      PrimeTester pt (gen_to_pt, pt_to_fact, output);
      Factorizer fact (pt_to_fact, output);

      gen.activate (n_generators, true);
      pt.activate (n_primetesters, true);
      fact.activate (n_factorizers, true);

      while (n_primes > pt.primes_found ())
        {
          std::cout << output.dequeue () << std::endl;
        }

      gen.cancel (); pt.cancel (); fact.cancel ();
    }
  catch (std::exception  &ex)
    {
      std::cerr << "Caught exception: " << ex.what () << std::endl;
      return -1;
    }
  catch (...)
    {
      std::cerr << "Caught unknown exception." << std::endl;
      return -1;
    }
  return 0;
 }
