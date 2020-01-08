#ifndef CROSS_H
#define CROSS_H

class Cross
{
public:
    int id;
    int roadId1;
    int roadId2;
    int roadId3;
    int roadId4;

    static int count;

public:
    Cross(int info[])
    {
        this->id = info[0];
        this->roadId1 = info[1];
        this->roadId2 = info[2];
        this->roadId3 = info[3];
        this->roadId4 = info[4];

        count++;
    }
};

#endif // CROSS_H
