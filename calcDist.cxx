#include <stdlib.h>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>

constexpr int alignment = 32 ;

/* Aligned Dyn Array */
template<typename T,int Alignment>
class AlignedDynArray
{
public:
  AlignedDynArray() = delete;
  AlignedDynArray(AlignedDynArray const&) = delete;
  AlignedDynArray& operator=(AlignedDynArray const&) = delete;
  AlignedDynArray(AlignedDynArray&&) = delete;
  AlignedDynArray& operator=(AlignedDynArray&&) = delete;

  explicit AlignedDynArray(size_t n);
  ~AlignedDynArray();

  ///conversions to T*
  operator T*(); 
  operator const T*() const; 

  /// index array operators  
  T& operator [](const std::size_t pos);
  const T& operator [](const std::size_t pos) const ;

  ///size of allocated buffer
  std::size_t size() const;
private:
  T*  m_buffer=nullptr;
  void* m_raw=nullptr;
  size_t const m_size=0;
}; 
template<typename T, int Alignment>
inline
AlignedDynArray<T,Alignment>::AlignedDynArray(size_t n): m_buffer(nullptr), 
  m_raw(nullptr),
  m_size(n){
    size_t const size = n * sizeof(T) +alignof(T) ;
    //create buffer of right size,properly aligned
    posix_memalign(&m_raw, Alignment, size);
    //placement new of elements to the buffer
    m_buffer = new (m_raw) T[n];
  }

template<typename T, int Alignment>
inline  
AlignedDynArray<T,Alignment>::~AlignedDynArray(){
  for (std::size_t pos = 0; pos < m_size; ++pos) {
    m_buffer[pos].~T();
  }
  free(m_buffer);
}

template<typename T, int Alignment>
inline   
AlignedDynArray<T,Alignment>::operator T*() {return m_buffer;} 

template<typename T, int Alignment>
inline   
AlignedDynArray<T,Alignment>::operator const T*() const {return m_buffer;}

template<typename T, int Alignment>
inline  
T& AlignedDynArray<T,Alignment>::operator[] (const std::size_t pos) { return m_buffer[pos];}

template<typename T, int Alignment>
inline  
const T& AlignedDynArray<T,Alignment>::operator[] (const std::size_t pos) const { return m_buffer[pos];}

template<typename T, int Alignment>
inline  
std::size_t AlignedDynArray<T,Alignment>::size() const {return m_size;}
/* end of AlignedDynArray*/

  void
calculateAllDistances(float* __restrict qonpIn,
                      float* __restrict qonpCovIn,
                      float* __restrict qonpGIn,
                      float* __restrict distancesIn,
                      size_t n)
{
  n = n&0xfffffff0; //n is multiple of 16  
  float* qonp = (float*)__builtin_assume_aligned(qonpIn, alignment);
  float* qonpCov = (float*)__builtin_assume_aligned(qonpCovIn, alignment);
  float* qonpG = (float*)__builtin_assume_aligned(qonpGIn, alignment);
  float* distances = (float*)__builtin_assume_aligned(distancesIn, alignment);
  for (size_t i = 0; i < n; ++i) {
    size_t indexConst = (i + 1) * i / 2;
    float qonpi = qonp[i];
    float qonpCovi = qonpCov[i];
    float qonpGi = qonpG[i];
    for (size_t j = 0; j < i; ++j) {
      float parametersDifference = qonpi - qonp[j];
      float covarianceDifference = qonpCovi - qonpCov[j];
      float G_difference = qonpG[j] - qonpGi;
      float G_sum = qonpGi + qonpG[j];
      distances[indexConst + j] =covarianceDifference * G_difference + parametersDifference * G_sum * parametersDifference;
      //\log \frac{\sigma_2}{\sigma_1} + \frac{\sigma_1^2 + (\mu_1 - \mu_2)^2}{2 \sigma_2^2} - \frac{1}{2}
    }
  }
}

int main(){
  /*
   * Fill array with random numbers
   */
  //std::random_device rd;
  //std::mt19937 gen(rd()); 
  std::mt19937 gen; 
  std::uniform_real_distribution<> dis(1.0, 5.0);
  const size_t n= 72;
  const size_t initnn= n*(n+1)/2;
  const size_t nn = ( 16* round(initnn /16. ));
  AlignedDynArray<float,alignment> distancesIn(nn);
  AlignedDynArray<float,alignment> qonpIn(nn);
  AlignedDynArray<float,alignment> qonpCovIn(nn);
  AlignedDynArray<float,alignment> qonpGIn(nn);
  std::cout << "size " << nn <<std::endl;
  for (size_t i = 0; i < n; ++i) {
    qonpIn[i]=dis(gen);
    qonpCovIn[i]=dis(gen);
    qonpGIn[i]=dis(gen);
  }
  for (size_t i = 0; i < nn; ++i) {
    distancesIn[i]=0;
  }

  {
    //Test simple C-style solution
    std::cout << "-- calculateAllDistances ---" <<'\n';
    std::chrono::steady_clock::time_point clock_begin = std::chrono::steady_clock::now(); 
    calculateAllDistances(qonpIn,qonpCovIn,qonpGIn,distancesIn,n);
    std::chrono::steady_clock::time_point clock_end = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration diff = clock_end - clock_begin;
    //print 
    std::cout << distancesIn[1] << " , "  <<distancesIn[2] << " , " << distancesIn[3] << " , "  <<distancesIn[4] <<'\n';
    std::cout <<"Time: " << std::chrono::duration <double, std::nano> (diff).count() << "ns" << '\n';
  } 

  std::cout<<'\n';

  return 0;
}