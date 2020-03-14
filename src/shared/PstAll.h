#ifndef PSTALL_H
#define PSTALL_H

#include <QString>
#include <map>

struct PstAllProps
{
    int SID;
    float exc;
    float inh;
};

class PstAll
{
  public:
    void load(QString filename);
    PstAllProps get(int SID);

  private:
    std::map<int, PstAllProps> mPstAll;
};

#endif