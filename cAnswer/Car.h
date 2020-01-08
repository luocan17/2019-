#ifndef CAR_H
#define CAR_H

#include <vector>
#include <set>

class Car
{
public:
    int id;
    int from;
    int to;
    int speed;
    int planTime;

    static int count;
    static std::set<int> speedSet;

    std::vector<int> answer;

public:
    Car(int info[])
    {
        this->id = info[0];
        this->from = info[1];
        this->to = info[2];
        this->speed = info[3];
        this->planTime = info[4];

        count++;
        speedSet.insert(speed);

        answer.push_back(id);
        answer.push_back(planTime);
    }
};

#endif // CAR_H
