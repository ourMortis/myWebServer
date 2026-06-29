#ifndef TASK_HPP
#define TASK_HPP

class Task
{
public:
    virtual void process() = 0;
    
    virtual ~Task() = default;
};



#endif // TASK_HPP