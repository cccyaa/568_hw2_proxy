#ifndef EXCEPTION
#define EXCEPTION


class mainException: public exception
{
  public:
  	virtual const char* what() const throw()
  {
    return "Main thread error\n";
  }
}; 

// class childException: public exception
// {
// public:
//   virtual const char* what() const throw()
//   {
//     return "Child thread error\n";
//   }
// };

class getException: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "GET/POST error\n";
  }
};

class connectException: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "CONNECT error\n";
  }
};
#endif
