#include <unistd.h>
#include <strings.h>
#include "malloc.h"

typedef struct free_list{
	int size;
	struct free_list *next;
	struct free_list *before;
} *Free_list;

int found;

Free_list f = NULL;
Free_list a = NULL;

void *malloc(unsigned int size)
{
	char *p;

	if (size == 0)
	{
		return NULL;
	}

	// Первый вызов
	if (f == NULL)
	{
		// Выделяем память с 12 байт для заголовка
		// Если меньше 3, 10*size мало поэтому выделяем 40.
		if (size > 3)
		{
			p = sbrk(10*size);
			f = (void *) p;
			f -> size = 10*size - 12;
		}
		else
		{
			p = sbrk(40);
			f = (void *) p;
			f -> size = 40 - 12;
		}
		f -> next = NULL;
		f -> before = NULL;

		p = (void*) &(f -> size);

		if (*p == -1)
		{
			return NULL;
		}

		// 8 байт заголовок после (p + 12) который имеет фактический адрес и чексумму.
		*(p + 12) = size;	// Actual size to be returned.
		*(p + 12 + 4) = 0;	// Checksum.

		// кусок недоступен.
		f -> size = 0;

		// инициализация атрибутов.
		f -> next = (void *) p + 20 + size;

		if (size > 3)
			f -> next -> size =  10*size - (12 + 8 + size + 12);
		else
			f -> next -> size =  40 - (12 + 8 + size + 12);
		f -> next -> next = NULL;
		f -> next -> before = (void *) p;
 
 		// возвращаем место.
 		return p + 20;

	}
	// вызов не первый раз.
	else
	{
		found = 1;

		// перемещение по листу.
		a = f;

		// пока размер больше того, который доступен.
		while (a -> size <= (size + 20))
		{
			// если есть следущий блок,переход.
			if (a -> next != NULL)
			{
				a = a -> next;
			}
			// нет места.
			else
			{
				found = 0;
				break;
			}
		}

		// место найдено в буфере.
		if (found)
		{
			p = (void*) &(a -> size);

			// 12 байт уже выделено предыдущим вызовом.

			//выделяем 8байт.
			*(p + 12) = size;	// размер для возврата.
			*(p + 12 + 4) = 0;	// Чексумма.

			if (a -> next == NULL)
			{
				a -> next = (void *) p + 12 + 8 + size;
				a -> next -> size =  a -> size - (8 + size + 12);
				a -> next -> next = NULL;
				a -> next -> before = (void *) a;
			}

			a -> size = 0;
			return p + 20;
		}
		// Нет места в буфере.
		else
		{
			// Allocate more memory.
			// If size is less than 3, 10*size is not enough so allocate 40.
			if (size > 3)
			{
				p = sbrk(10*size);
			}
			else
			{
				p = sbrk(40);
			}

			if (*p == -1)
			{
				return NULL;
			}

			// Make sure the last chunk is now pointing to the beginning of the newly allocated memory.
			a -> next = (void *) p;
			a = a -> next;

			a = (void *) p;

			// 12 byte header.
			if (size > 3)
			{
				a -> size = 10*size - 12;
			}
			else
			{
				a -> size = 40 - 12;
			}

			p = (void*) &(a -> size);

			a -> next = NULL;
			a -> before = NULL;


			// 8 byte header after (p + 12) which has the actual size to be used and the checksum.
			*(p + 12) = size;	// Actual size to be returned.
			*(p + 12 + 4) = 0;	// Checksum.

			// This chunk is not available any more.
			a -> size = 0;

			// Set the next attributes.
			a -> next = (void *) p + 20 + size;
			if (size > 3)
			{
				a -> next -> size =  10*size - (12 + 8 + size + 12);
			}
			else
			{
				a -> next -> size =  40 - (12 + 8 + size + 12);
			}
			a -> next -> next = NULL;
			a -> next -> before = (void *) a;

	 		// Return the space requested from user.
	 		return p + 20;
		}
	}
	return NULL;
}

void free(void *p)
{
	int size;
	char *a;
	a = (void *)p;

	// Cannot free a NULL pointer.
	if(p == NULL)
		return;

	// If checksum is indeed 0.
	if (*(a - 4) == 0 && (a - 8) != NULL && (a - 20) != NULL)
	{
		size = *(a - 8);
		*(a- 20) = size + 8; // Size + 8 byte headers available to use again.
	}
}

void *calloc(size_t nmemb, size_t size)
{
	char *p;

	// If zero return NULL.
	if (nmemb == 0 || size == 0)
	{
		return NULL;
	}

	// Malloc the area and zero it out.
	else
	{
		p = malloc(nmemb * size);
		bzero(p, nmemb * size);
		return p;
	}
}


void *realloc(void *ptr, size_t size)
{
	char *p;
	char *a = ptr;

	// Realloc on a NULL pointer same as malloc.
	if (ptr == NULL)
	{
		p = malloc(size);
		return p;
	}

	// Realloc to size 0 same as free.
	else if (size == 0)
	{
		free(ptr);
		return NULL;
	}

	// Cannot realloc to size smaller.
	else if(size > *(a - 8)){
		return NULL;
	}

	// Malloc and copy to given pointer, then free the temporary portion.
	else
	{
		p = malloc(size);
		bcopy(ptr, p, size);
		free(p);
		return p;
	}
}
