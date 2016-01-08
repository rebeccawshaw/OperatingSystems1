#ifndef SORT_STRATEGY_TXX
#define SORT_STRATEGY_TXX

#include "utility.hxx"
#include <exception>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <system_error>

namespace po = boost::program_options;
extern po::variables_map vm;
namespace cs281
{
  template< typename T >
  Sort_Strategy<T> *
  Sort_Strategy<T>::get_strategy (const std::string type)
  {
    if (type == "insertion")
      return new Insertion_Sort<T> ();
    if (type == "gnusort1")
      return  new gnusort1<T> ();
    if (type == "gnusortn")
          return new gnusortn<T> ();

    throw std::runtime_error ("Invalid sort strategy\n");
  }

  template< typename T >
  void
  Insertion_Sort<T>::sort (std::vector< T > &data)
  {
    insertion_sort (data);
  }
  template< typename T >
   void
   gnusort1<T>::sort (std::vector< T > &data)
   {
    const char *t = "temp";
    File<int> temp(t);
    const char *sT = "sortedTemp";
	File<int> result(sT);
			
    temp.write(data);
    
    pid_t rc = fork();
    pid_t w;
    int status;
    
    if (rc < 0) {
        std::cout << "Exception thrown." << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (rc == 0) {      // Code executed by child
        char *ch[6];
        ch[0] = strdup("sort");
        ch[1] = strdup("-n");
        ch[2] = strdup(t);
        ch[3] = strdup("-o");
        ch[4] = strdup(sT);
        ch[5] = nullptr;
        execvp(ch[0], ch);
        //! Yogesh: what if the exec fails?
    } else {                  // Code executed by parent
        w = waitpid(rc, &status, WUNTRACED);
        if(!WIFEXITED(status)) {
			std::cerr << "Child Error" << std::endl;
		} else {
			data.clear();
			result.read(data);
		}
    }
   }
   
  template< typename T >
   void
   gnusortn<T>::sort (std::vector< T > &data)
   {
    int numparallel=vm["num-parallel"].as< int > ();
    
    // Write input data to n temp files
    int size = data.size() / numparallel;
    typename std::vector<T>::iterator iter = data.begin();
    
    for (int i = 0; i < numparallel; i++) {
        std::string f = "temp" + std::to_string(i);
        File<int> file(f);
        if (i != numparallel -1) {
            file.write(iter, iter + size);
            iter += size;
        } else file.write(iter, data.end());
    }
    
    // Spawn n children to each temporary file and sort
    pid_t rc, pid[numparallel];
    int status[numparallel];
    
    
    for (int i = 0; i < numparallel; i++) {
        rc = fork();
        
        if (rc < 0) {
            std::cerr << "Exception thrown." << std::endl;
            
            exit(EXIT_FAILURE);
        } else if (rc == 0) {
            std::string t = "temp" + std::to_string(i);
            std::string sT = "sortedTemp" + std::to_string(i);
            char *ch[6];
            ch[0] = strdup("sort");
            ch[1] = strdup("-n");
            ch[2] = strdup(t.c_str());
            ch[3] = strdup("-o");
            ch[4] = strdup(sT.c_str());
            ch[5] = nullptr;
            execvp(ch[0], ch);
            
            exit(EXIT_SUCCESS);
        } else {
        	        //! TODO:@Yogesh
        // This needs to be outside the for loop. having this code inside the for loop
        // causes the code to wait untill the child has finished execution, thus causing sequential execution.
        // once done it will then move to the next iterator in the for loop to spawn next process

            pid[i] = waitpid(rc, &status[i], WUNTRACED);
            
            if(!WIFEXITED(status[i])) {
                std::cerr << "Exception" << std::endl;
            }
        }
    }
    // Spawn child to merge output
    rc = fork();
    pid_t w;
    int stat;
    
    std::string sorted = "sortedTemp";
    
    if (rc < 0) {
        std::cerr << "Exception." << std::endl;
        exit(EXIT_FAILURE);
    } else if (rc == 0) {
        char *ch[6 + numparallel];
        ch[0] = strdup("sort");
        ch[1] = strdup("-n");
        ch[2] = strdup("-m");
        ch[3] = strdup("-o");
        ch[4] = strdup(sorted.c_str());
        for (int i = 5; i < numparallel + 5; i++) {
            std::string sort = sorted + std::to_string(i - 5);
            ch[i] = strdup(sort.c_str());
        }
        ch[5 + numparallel] = nullptr;
    
        execvp(ch[0], ch);
    } else {
        w = waitpid(rc, &stat, WUNTRACED);

        if(!WIFEXITED(stat)) {
            std::cerr << "Exception!" << std::endl;
        } else {
            File<int> ans(sorted);
            data.clear();
            ans.read(data);
        }
    }           
   }

}

#endif
