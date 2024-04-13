#pragma once

#include <vector>
#include <queue>

using namespace std;


/** \brief Ping pong buffer used for multi-threaded comminication
    betweent different components using dynamic vectors. */
template <typename T>
class PP_Buff {
public:
  /** \brief Gets the communication buffer for the cycle. */
  vector<T> &get_comm_buff(uint64_t cycle) {
    if(cycle%2 == 0)
      return buff1;
    else
      return buff2;
  }
  /** \brief Gets the computational buffer for the cycle. */
  vector<T> &get_comp_buff(uint64_t cycle) {
    if(cycle%2 == 0)
      return buff2;
    else
      return buff1;
  }
private:
  vector<T> buff1;
  vector<T> buff2;
};


/** \brief Ping pong buffer with fixed capacity used for
    multi-threaded comminication betweent different components.

    In comparaison with the PP_Buff, this class has a fixed size. This
    alows us to use OpenMP atomic operation, instead of the critical
    section, needed by the PP_buff.
*/
template <typename T>
class PP_static_Buff {
public:
  PP_static_Buff(int size) : size(size), nb_1(0), nb_2(0) {
    buff1 = new T[size];
    buff2 = new T[size];
  }

  ~PP_static_Buff() {
    delete buff1;
    delete buff2;
  }

  /** \brief Inserts the elem in the communication buffer. Returns
      true if success and false if there is no more space. */
  bool insert(uint64_t cycle, T elem) {
    int pos;
    
    if(cycle%2 == 0)
#pragma omp atomic capture
      pos = nb_1++;
    else
#pragma omp atomic capture
      pos = nb_2++;
    
    if (pos >= size)
      return false;
    
    if(cycle%2 == 0)
      buff1[pos] = elem;
    else
      buff2[pos] = elem;
    return true;
  }
  /** \brief Gets the computational buffer for the given cycle.

      The number of elements for the buffer is reset to 0.
   */
  T *get_comp_buff(uint64_t cycle) {
    if(cycle%2 == 0) {
      nb_2 = 0;
      return buff2;
    } else {
      nb_1 = 0;
      return buff1;
    }
  }

  /** Get the number of new elements. */
  int get_comp_elems(int cycle) {
    int ret;
    if(cycle%2 == 0)
      ret = (nb_2 > size) ? size : nb_2; 
    else
      ret = (nb_1 > size) ? size : nb_1;
    return ret;
  }
  
private:
  T *buff1;
  T *buff2;
  int size;
  int nb_1;
  int nb_2;
};
