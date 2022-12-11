typedef struct vest
{
    char tema[20];
    char poruka[100]; 
}VEST;

int rear = - 1;
int front = - 1;

void insert(VEST vest, VEST** dataBuffer)
{
    if(rear == 9)
    {
        printf("Queue Overflow n");
    }
    else
    {
        if(front== - 1)
        front = 0;
        rear = rear + 1;
        *dataBuffer[rear] = vest;
    }
}

void delete(VEST vest, VEST** dataBuffer)
{
    if(front == - 1 || front > rear)
    {
        printf("Queue Underflow n");
        return;
    }
    else
    {
        printf("Element deleted from queue is : %s\n", *dataBuffer[front]->poruka);
        front = front + 1;
    }
}
