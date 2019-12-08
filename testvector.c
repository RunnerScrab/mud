#include "vector.h"
#include "talloc.h"
#include <stdio.h>
#include <string.h>

struct TestClient
{
  char name[64];
  unsigned int age;
};

struct TestClient* TestClient_Create(char* n, unsigned int a)
{
  struct TestClient* pNew = (struct TestClient*) talloc(sizeof(struct TestClient));
  memset(pNew->name, 0, 64);
  strncpy(pNew->name, n, 63);
  pNew->age = a;
  return pNew;
}

int compclients(void* a, void* b)
{
  int key = *((int*)a);
  struct TestClient *pb = 0;
  pb = (struct TestClient*) b;
  return key - pb->age;
}

void swap(void** a, void** b);

int main(void)
{
  printf("size of int: %d size of void*: %d\n",
	 sizeof(int), sizeof(void*));
  int i = 0, z = 0;
  struct Vector clients;
  Vector_Create(&clients, 32);
  for(z = 32; i < z; ++i)
    {
      Vector_Push(&clients, (void*) TestClient_Create("Paul", i));
    }
  int searchkey = 16;
  size_t iFound = 0;

  if(Vector_Find(&clients, &searchkey, compclients, &iFound) >= 0)
    Vector_Remove(&clients, iFound);


    searchkey = 30;
  if(Vector_Find(&clients, &searchkey, compclients, &iFound) >= 0)
    Vector_Remove(&clients, iFound);

  
  
  
  for(i = 0; i < clients.fill_pointer; ++i)
    {
      struct TestClient* pClient = (struct TestClient*) Vector_At(&clients, i);
      printf("%s %d\n", pClient->name, pClient->age);
    }
  printf("Destroying.\n");
  Vector_Destroy(&clients);
  printf("%d outstanding allocs.\n", toutstanding_allocs());
  return 0;
}
