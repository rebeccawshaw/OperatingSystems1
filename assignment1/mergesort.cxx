#include <iostream>
#include <random>
#include <functional>
#include <chrono>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <unistd.h>
#include "utility.hxx"
#include "sort_strategy.hxx"

namespace po = boost::program_options;
po::variables_map vm;

int main (int argc, char **argv)
{
  try
  {
      // Parse program options
      po::options_description desc("Allowed options");
      desc.add_options()
		    ("help", "Produce this help message")
		    ("num-parallel,n", po::value<int>(), "number of parallel processes to run (only valid with gnusortn)")
		    ("strategy,s", po::value<std::string>(), "Sorting strategy: insertion, gnusort1, gnusortn")
		    ("input-file,i", po::value<std::string>(), "Input file")
		    ("output-file,o", po::value<std::string>(), "Output file.  Default: input_file_name.sorted")
		    ("verbose,v", "Generate verbose logging")
		    ;


      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);

      if (vm.count("help")) {
          std::cout << desc << "\n";
          return 0;
      }

      std::string strategy;
      std::vector<int> data;

      if (!vm.count ("strategy"))
        {
          std::cerr << "No valid strategy defined!\n";
          return -1;
        }
      strategy=vm["strategy"].as < std::string > ();


      if(strategy=="gnusortn" && !vm.count ("num-parallel"))
        {
          std::cerr<<"num-parallel must be given with gnusortn!\n";
          return -1;
        }
      ;

      if (strategy=="gnusortn" && (vm["num-parallel"].as< int > () < 1 || vm["num-parallel"].as< int > ()> 8))
        {
          std::cerr << "cannot run " << vm["num-parallel"].as< int > ()
              << " concurrent sort. num-parallel should be between 1 and 8!\n";
          return -1;
        }
      if (!vm.count ("input-file"))
        {
          std::cerr << "No input file given!\n";
          return -1;
        }
      else if(access(vm["input-file"].as < std::string > ().c_str(), R_OK)!=0)
        {
          std::cerr<<"cannot access "<< vm["input-file"].as < std::string > ()<<"!\n";
          return -1;
        }
      if (vm.count ("verbose")) std::cout << "Reading data from file "
          <<  vm["input-file"].as < std::string > () << std::endl;

      File<int> file1 (vm["input-file"].as < std::string > ());
      file1.read (data);

      if (vm.count ("verbose")) std::cout << "Read in " << data.size () << "records\n";





      std::unique_ptr<cs281::Sort_Strategy<int> > sort_strategy
      (cs281::Sort_Strategy<int>::get_strategy (vm["strategy"].as < std::string > ()));
      sort_strategy->sort (data);
      //validation of the sorted data
      if (!std::is_sorted (data.begin (), data.end ()))
        {
          std::cerr << "Error:  Array is not sorted!\n";
        }

      File<int> file (vm.count ("output-file") ? vm["output-file"].as < std::string > () :
          (vm["input-file"].as < std::string > () + ".sorted"));
      file.write (data);
  }
  catch (std::exception &ex)
  {
      std::cerr << "Caught fatal exception: " << ex.what () << std::endl;
      return -1;
  }

  return 0;
}