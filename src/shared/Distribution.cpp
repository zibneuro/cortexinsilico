#include "Distribution.h"

Distribution::Distribution(){};

void Distribution::addSample(double value)
{
    mData.push_back(value);
};

QJsonArray Distribution::getJson()
{
    QJsonArray array;
    for (auto it = mData.begin(); it != mData.end(); it++)
    {
        array.push_back(*it);
    }
    return array;
}