#include "textutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "utf8.h"
#include "charvector.h"
#include "bitutils.h"

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

//This file is going to be pretty big, because the algorithms here use one-off
//ADTs which are likely not going to be reused because they contain specific
//types. In C, we don't have templates/generics and must hand write every
//specialization.

typedef struct intpair
{
	int x, y;
} intpair_t;

typedef struct reflow_pairdeque
{
	intpair_t* data;
	size_t length, capacity;
} reflow_pairdeque_t;

static void reflow_pairdeque_create(struct reflow_pairdeque* stack, size_t initial)
{
	stack->data = (intpair_t*) malloc(sizeof(intpair_t) * initial);
	memset(stack->data, 0, sizeof(intpair_t) * initial);
	stack->capacity = initial;
	stack->length = 0;
}

static inline int reflow_pairdeque_popnoret(struct reflow_pairdeque* stack)
{
	if(stack->length > 0)
	{
		return --stack->length;
	}
	else
	{
		return -1;
	}

//Not clearing memory makes pop less correct, but still works correctly as used
//in this file and avoids repeated memsets in a tight loop
//memset(&stack->data[stack->length], 0, sizeof(intpair_t));
}

static inline void reflow_pairdeque_clear(struct reflow_pairdeque* stack)
{
//Not clearing memory makes clear less correct, but still works as used in this
//file and avoids repeated memsets in a tight loop
//memset(stack->data, 0, sizeof(intpair_t) * stack->capacity);
	stack->length = 0;
}

static inline int reflow_pairdeque_peek_x(struct reflow_pairdeque* stack)
{
	return stack->data[stack->length - 1].x;
}

static inline int reflow_pairdeque_peek_y(struct reflow_pairdeque* stack)
{
	return stack->data[stack->length - 1].y;
}


static void reflow_pairdeque_push(struct reflow_pairdeque* stack, int x, int y)
{
	if(stack->length >= stack->capacity)
	{
		stack->capacity <<= 2; //We can only push one value at a time through this function
		stack->data = (intpair_t*) realloc(stack->data, sizeof(intpair_t) * stack->capacity);
	}

	stack->data[stack->length].x = x;
	stack->data[stack->length].y = y;
	++stack->length;
}

static void reflow_pairdeque_destroy(struct reflow_pairdeque* stack)
{
	free(stack->data);
}

typedef struct reflow_word
{
	cv_t string;
	unsigned char bHyphenPoint;
	unsigned char bEscaped;
} reflow_word_t;

typedef struct reflow_strarray
{
	reflow_word_t* strings;
	size_t length;
	size_t capacity;
} reflow_strarray_t;

static int reflow_strarray_create(reflow_strarray_t* array, size_t initial_size);
static void reflow_strarray_push(reflow_strarray_t* array, char* val, size_t vallen,
				unsigned char bIsEscaped, unsigned char bIsHyphenPoint);
static void reflow_strarray_destroy(reflow_strarray_t* array);

typedef struct reflow_intstack
{
	int* data;
	size_t length;
	size_t capacity;
} reflow_intstack_t;

static void reflow_intstack_create(struct reflow_intstack* stack, size_t initial);
static int reflow_intstack_pop(struct reflow_intstack* stack);
static int reflow_intstack_peek(struct reflow_intstack* stack);
static void reflow_intstack_push(struct reflow_intstack* stack, int val);
static void reflow_intstack_destroy(struct reflow_intstack* stack);

static void FindParagraphs(const char* text, size_t length, struct reflow_intstack* paragraphlocs);
static void TokenizeString(const char* input, size_t inputlen, reflow_strarray_t* out, unsigned char bPermitHyphenation);
static void StripNewline(const char* input, size_t inputlen, char* out, size_t bufferlen);

static int reflow_strarray_create(struct reflow_strarray* array, size_t initial_size)
{
	array->length = 0;
	array->capacity = max(initial_size, 1);

	size_t strarrsize = sizeof(reflow_word_t) * array->capacity;
	array->strings = (reflow_word_t*) malloc(strarrsize);
	memset(array->strings, 0, strarrsize);

	return array->strings ? 0 : -1;
}

static void reflow_strarray_push(struct reflow_strarray* array, char* val, size_t vallen,
				unsigned char bIsEscaped, unsigned char bIsHyphenPoint)
{
	if(array->length >= array->capacity)
	{
		array->capacity = max((array->capacity<<3), (array->capacity + array->length));
		size_t strarrsize = sizeof(reflow_word_t) * array->capacity;
		array->strings = (reflow_word_t*) realloc(array->strings, strarrsize);
		memset(&array->strings[array->length], 0, sizeof(reflow_word_t) * (array->capacity - array->length));
	}
	reflow_word_t* pword = &array->strings[array->length];
	pword->bHyphenPoint = bIsHyphenPoint;
	pword->bEscaped = bIsEscaped;
	cv_init(&pword->string, 16);
	cv_appendstr(&pword->string, val, vallen);

	++array->length;
}

static void reflow_strarray_destroy(struct reflow_strarray* array)
{
	size_t idx = 0;
	for(; idx < array->length; ++idx)
	{
		cv_destroy(&array->strings[idx].string);
	}
	free(array->strings);
}

static void reflow_intstack_create(struct reflow_intstack* stack, size_t initial)
{
	stack->capacity = max(1, initial);
	stack->data = (int*) malloc(sizeof(int) * stack->capacity);
	stack->length = 0;
}

static int reflow_intstack_pop(struct reflow_intstack* stack)
{
	if(stack->length > 0)
	{
		--stack->length;
	}

	int retval = stack->data[stack->length];
	stack->data[stack->length] = 0;

	return retval;
}

static int reflow_intstack_peek(struct reflow_intstack* stack)
{
	return stack->data[stack->length - 1];
}

static void reflow_intstack_push(struct reflow_intstack* stack, int val)
{
	if(stack->length >= stack->capacity)
	{
		stack->capacity <<= 2; //We can only push one value at a time through this function
		stack->data = (int*) realloc(stack->data, sizeof(int) * stack->capacity);
	}

	stack->data[stack->length] = val;
	++stack->length;
}

static void reflow_intstack_destroy(struct reflow_intstack* stack)
{
	free(stack->data);
}


static void SplitWord(const char* inword, size_t inwordlen, cv_t* outword_a, cv_t* outword_b)
{
	size_t halflen = inwordlen / 2;
	cv_strncpy(outword_a, inword, halflen);
	cv_push(outword_a, 0);
	cv_strncpy(outword_b, &inword[halflen], inwordlen - halflen);
	cv_push(outword_b, 0);
}

static inline unsigned int CanWordBeSplit(const char* word, size_t len)
{
	if(len < 8 || isupper(word[0]) || word[0] == '"' || memchr(word, '-', len) ||
		utf8findstart(word, len))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

static void TokenizeString(const char* input, size_t len, reflow_strarray_t* out, unsigned char bCreateHyphenPoints)
{
	char* savep = 0;
	char* ret = 0;
	char* inputcopy = (char*) malloc(sizeof(char) * (len +1));
	memcpy(inputcopy, input, sizeof(char) * (len));
	inputcopy[len] = 0;
	size_t offset = 0, lastescapetoken = 0, wordlen = 0;
	unsigned char spacealignment = 0;

	cv_t rebuilttoken;
	cv_init(&rebuilttoken, 32);
	cv_t a, b;
	cv_init(&a, 16);
	cv_init(&b, 16);

	do
	{
		ret = strtok_r(ret ? 0 : inputcopy, " `", &savep);

		if(ret && ret >= &inputcopy[len - 1])
		{
			free(inputcopy);
			return;
		}
		else if(ret)
		{
			offset = ret - inputcopy;
			wordlen = strnlen(ret, &input[len] - ret);
			if(offset > 0 && input[offset - 1] == '`' && lastescapetoken != offset - 1 &&
				(offset + wordlen) < len && input[offset + wordlen] == '`')

			{
				//Special handling of escaped words. Escaped words are not displayed, though
				//they perform control functions (enabling color, bold, italics, etc.).
				//Here we store metadata for them for the reflow algorithm to later use when
				//placing words down.

				cv_sprintf(&rebuilttoken, "`%s`", ret);

				spacealignment = 1;
				//bit 1 set for any escaped word, bit 2 set for space on the left, bit 4 set for space on the right
				if((offset - 2) > 0 && isspace(input[offset - 2]))
					spacealignment |= 2;
				if((offset + wordlen + 1) < len &&
					isspace(input[offset + wordlen + 1]))
					spacealignment |= 4;

				reflow_strarray_push(out, rebuilttoken.data,
						rebuilttoken.length, spacealignment, 0);

				lastescapetoken =  offset + wordlen;
			}
			else if(bCreateHyphenPoints && CanWordBeSplit(ret, wordlen))
			{

				SplitWord(ret, wordlen, &a, &b);
				//By default, the word's cost is its length.
				reflow_strarray_push(out, a.data, a.length, 0, 1);
				reflow_strarray_push(out, b.data, b.length, 0, 0);
			}
			else
			{
				reflow_strarray_push(out, ret, wordlen, 0, 0);
			}
		}
	}
	while(ret);

	cv_destroy(&a);
	cv_destroy(&b);
	cv_destroy(&rebuilttoken);
	free(inputcopy);
}

static void FindParagraphs(const char* text, size_t length, struct reflow_intstack* paragraphlocs)
{
	const char* p = &text[strspn(text, " ")];

	reflow_intstack_push(paragraphlocs, p - text);
	const char* found = 0;
	do
	{
		found = strchr(p, '\n');
		if(found)
		{
			size_t nls_found = 1;
			const char* nlrun = found;
			for(; *nlrun && *nlrun == '\n'; ++nlrun, ++nls_found);
			if(nls_found > 1)
			{
				//We have encountered at least one blank line, which separates
				//paragraphs
				reflow_intstack_push(paragraphlocs, found - text - 1); // Push end of last paragraph
				reflow_intstack_push(paragraphlocs, nlrun - text); //Push start of new one

			}
			p = nlrun;
		}
	}
	while(found);
}

static inline void PerformReflow(const reflow_strarray_t* words, const int* breaks, cv_t* output)
{
	size_t count = words->length, idx = 0;
	int i = 0, j = count;
	reflow_intstack_t revbreak;
	reflow_intstack_create(&revbreak, count);

	reflow_intstack_push(&revbreak, count);
	while(j > 0)
	{
		i = breaks[j];
		reflow_intstack_push(&revbreak, i);
		if(j == i)
		{
			break;
		}
		j = i;
	}
	i = reflow_intstack_pop(&revbreak);

	reflow_word_t* pword = 0;
	unsigned char spaceescapeflag = 0;
	cv_t wordbuf;
	cv_init(&wordbuf, 64);
	do
	{
		j = reflow_intstack_pop(&revbreak);

		for(idx = i; idx < j; ++idx)
		{
			pword = &words->strings[idx];
			spaceescapeflag = pword->bEscaped;
			if(!pword->bHyphenPoint)
			{
				//bit 1 set for any escaped word, bit 2 set for space on the left, bit 4 set for space on the right
				if(spaceescapeflag)
				{
					cv_sprintf(&wordbuf, "%*s%s%*s",
						(!!(spaceescapeflag & 2) && i != idx), "", pword->string.data,
						!!(spaceescapeflag & 4), "");
					cv_swap(&wordbuf, &pword->string);
				}
				else if(!words->strings[idx + 1].bEscaped)
				{
					cv_strncat(&pword->string, " ", 2);
				}
			}

			cv_strncat(output, pword->string.data, pword->string.length);
		}
		if((idx - 1) < words->length && words->strings[idx - 1].bHyphenPoint)
		{
			cv_strncat(output, "-", 2);
		}
		cv_strncat(output, "\n", 2);

		i = j;
	}
	while(j);


	cv_destroy(&wordbuf);
	reflow_intstack_destroy(&revbreak);
}

static void ReflowParagraph(const char* text, size_t len, const int width, cv_t* output, unsigned char bIndentFirstWord,
	unsigned char bAllowHyphenation)
{
	//Uses a shortest paths method to solve optimization problem

	if(len <= width)
	{
		cv_strncat(output, text, len);
		const char* p = lastnonspace(output->data,
					strnlen(output->data, output->length));
		if(p)
		{
			int offset = (p - output->data) + 1;
			if(offset < output->length)
			{
				output->data[offset] = '\n';
			}
		}
		return;
	}

	reflow_strarray_t words;
	reflow_strarray_create(&words, 64);

	TokenizeString((char*) text, len, &words, bAllowHyphenation);

	if(bIndentFirstWord)
	{
		cv_t spaced;
		cv_init(&spaced, words.strings[0].string.length + 2);
		cv_sprintf(&spaced, "%*s%s", bIndentFirstWord, "", words.strings[0].string.data);
		cv_swap(&spaced, &words.strings[0].string);
		cv_destroy(&spaced);
	}

	size_t idx = 0;
	size_t count = words.length;

	struct reflow_intstack offsets;
	reflow_intstack_create(&offsets, count + 1);
	reflow_intstack_push(&offsets, 0);

	//Calculate word costs. Words which do not represent a control sequence have a
	//cost simply equal to their length.
	for(idx = 0; idx < count; ++idx)
	{
		const reflow_word_t* pword = &words.strings[idx];
		reflow_intstack_push(&offsets, reflow_intstack_peek(&offsets) +
				(pword->bEscaped ? 0 : utf8strnlen(pword->string.data, pword->string.length)));
	}

	int* minima = (int*) malloc(sizeof(int) * (count+1));
	int* breaks = (int*) malloc(sizeof(int) * (count + 1));
	int wsincelasthyphen = 0;
	memset(breaks, 0, sizeof(int) * (count + 1));
	minima[0] = 0;
	memset(&minima[1], 0x7F, sizeof(int) * count);

	int i = 0, j = 0, w = 0, cost = 0;
	unsigned char thishyphenpoint = 0;
	const int hyphenspace = width << 1;
	for(; i <= count; ++i)
	{
		for(j = i + 1; j <= count; ++j)
		{
			thishyphenpoint = words.strings[j].bHyphenPoint;

			if(j < count)
			{
				w = offsets.data[j] - offsets.data[i] + j - i - 1;
			}
			else
			{
				//We don't need to optimize for the alignment of the last line
				w = width;
			}

			if(w > width)
			{
				break;
			}

			register int k = width - w;
			cost = minima[i] + (k * k);

			if(cost < minima[j])
			{
				if(!thishyphenpoint || (thishyphenpoint && wsincelasthyphen > hyphenspace))
				{
					//We don't want to end multiple lines in a row on a hyphen
					if(thishyphenpoint)
					{
						wsincelasthyphen = 0;
					}
					else
					{
						wsincelasthyphen += width;
					}

					minima[j] = cost;
					breaks[j] = i;
				}
			}

		}
	}

	PerformReflow(&words, breaks, output);

	reflow_intstack_destroy(&offsets);
	free(minima);
	free(breaks);
	reflow_strarray_destroy(&words);
}

static int costfn(int i, int j, const int* minima, const int* offsets, const int width, const int count)
{
	int w = j < count ? (offsets[j] - offsets[i] + j - i - 1) : width;

	if(w > width)
	{
		return  (w - width) << 16;
	}
	return minima[i] + (width - w) * (width - w);

}

static int hfn(int l, int k, const int* minima, const int* offsets, const int width, const int count)
{
	int low = l + 1;
	int high = count;
	int mid = 0;
	while(low < high)
	{
		mid = (low + high)>>1;
		if(costfn(l, mid, minima, offsets, width, count) <=
			costfn(k, mid, minima, offsets, width, count))
		{
			high = mid;
		}
		else
		{
			low = mid + 1;
		}
	}
	if(costfn(l, high, minima, offsets, width, count) <=
		costfn(k, high, minima, offsets, width, count))
	{
		return high;
	}
	return l + 2;
}

static void ReflowParagraphBinary(const char* text, size_t len,
				const int width, cv_t* output,
				unsigned char bIndentFirstWord, unsigned char bAllowHyphenation)
{
	if(len <= width)
	{
		cv_strncat(output, text, len);
		return;
	}
	reflow_strarray_t words;
	reflow_strarray_create(&words, 64);

	TokenizeString((char*) text, len, &words, bAllowHyphenation);

	if(bIndentFirstWord)
	{
		cv_t spaced;
		cv_init(&spaced, words.strings[0].string.length + 2);
		cv_sprintf(&spaced, "%*s%s", bIndentFirstWord, "", words.strings[0].string.data);
		cv_swap(&spaced, &words.strings[0].string);
		cv_destroy(&spaced);
	}

	size_t idx = 0;
	size_t count = words.length;

	struct reflow_intstack offsets;

	reflow_intstack_create(&offsets, count + 1);
	reflow_intstack_push(&offsets, 0);

	//Calculate word costs. Words which do not represent a control sequence have a
	//cost simply equal to their length.
	for(idx = 0; idx < count; ++idx)
	{
		const reflow_word_t* pword = &words.strings[idx];
		reflow_intstack_push(&offsets, reflow_intstack_peek(&offsets) +
				(pword->bEscaped ? 0 :
					utf8strnlen(pword->string.data, pword->string.length)));
	}

	int* minima = (int*) malloc(sizeof(int) * (count + 1));
	int* breaks = (int*) malloc(sizeof(int) * (count + 1));
	int wsincelasthyphen = 0;
	memset(breaks, 0, sizeof(int) * (count + 1));
	memset(minima, 0, sizeof(int) * (count + 1));

	struct reflow_pairdeque deque;
	reflow_pairdeque_create(&deque, count + 1);
	reflow_pairdeque_push(&deque, 1, 0);

	int j = 1, l = 0, peekx = 0, peeky = 0, mincost = 0;
	unsigned char thishyphenpoint = 0;
	const int hyphenspace = width << 1;
	for(; j <= count; ++j)
	{
		l = deque.data[0].x;
		mincost = costfn(j - 1, j, minima, offsets.data, width, count);
		if( mincost <
			costfn(l, j, minima, offsets.data, width, count))
		{
			minima[j] = mincost;
			breaks[j] = j - 1;
			reflow_pairdeque_clear(&deque);
			reflow_pairdeque_push(&deque, j - 1, j + 1);
		}
		else
		{
			thishyphenpoint = words.strings[j].bHyphenPoint;
			if(!thishyphenpoint || (thishyphenpoint && wsincelasthyphen > hyphenspace))
			{
				minima[j] = costfn(l, j, minima, offsets.data, width, count);
				breaks[j] = l;
				if(thishyphenpoint)
					wsincelasthyphen = 0;
				else
					wsincelasthyphen += width;
			}

			while(1)
			{
				peekx = reflow_pairdeque_peek_x(&deque);
				peeky = reflow_pairdeque_peek_y(&deque);

				if(costfn(j - 1, peekx, minima, offsets.data, width, count) >
					costfn(peekx, peeky, minima, offsets.data, width, count))
					break;
				if(reflow_pairdeque_popnoret(&deque) < 0)
					break;
			}

			reflow_pairdeque_push(&deque, j - 1,
					hfn(j - 1, reflow_pairdeque_peek_x(&deque), minima, offsets.data, width, count));
			if((j + 1) == deque.data[1].y)
			{
				reflow_pairdeque_popnoret(&deque);
			}
			else
			{
				++deque.data[0].y;
			}
		}
	}

	PerformReflow(&words, breaks, output);

	reflow_pairdeque_destroy(&deque);
	free(minima);
	free(breaks);
	reflow_intstack_destroy(&offsets);
	reflow_strarray_destroy(&words);
}

static void StripNewline(const char* input, size_t inputlen, char* out, size_t bufferlen)
{
	//Preserves blank lines
	const char* pos = input;
	const char* found = 0;
	const char* p = 0;
	size_t spaceleft = bufferlen - 1;
	size_t offset = 0;
	do
	{
		found = strchr(pos, '\n');
		offset = found - pos;
		if(found && (offset <= spaceleft))
		{
			p = found + 1;
			if(*p && '\n' == *p)
			{

				for(;*p && '\n' == *p; ++p);
				found = p - 1;
				offset = min(spaceleft, ((found - pos) + 1));
				//offset = min(spaceleft, (&p[min((strspn(p, "\n") - 1), (bufferlen - 1))] - pos + 1));
				strncat(out, pos, offset);
			}
			else
			{
				strncat(out, pos, offset);
				out[found - input] = ' ';
			}

			spaceleft -= offset + 1;
			pos = found + 1;
		}
		else if(found)
		{
			strncat(out, pos, spaceleft);
			break;
		}
		else
		{
			strncat(out, pos, &input[inputlen] - pos);
			spaceleft -= &input[inputlen] - pos;
		}
	}
	while(found);
}

typedef void (*ReflowParagraphFn)(const char*, size_t, const int, cv_t*, unsigned char, unsigned char);

static inline void ReflowTextImpl(const char* input, const size_t len, cv_t* output,
				const int width, unsigned char num_indent_spaces,
				unsigned char bAllowHyphenation, ReflowParagraphFn rpfn)
{
	char* nlstrippedbuf = (char*) malloc(sizeof(char) * (len + 1));
	memset(nlstrippedbuf, 0, sizeof(char) * (len + 1));
	StripNewline(input, len + 1, nlstrippedbuf, len + 1);

	reflow_intstack_t paragraphbounds;
	reflow_intstack_create(&paragraphbounds, 8);

	cv_t buffer;
	cv_init(&buffer, len);

	FindParagraphs(nlstrippedbuf, len, &paragraphbounds);

	size_t idx = 0;
	for(; idx < paragraphbounds.length; idx += 2)
	{
		int start = paragraphbounds.data[idx];
		int end = ((idx + 1) < paragraphbounds.length) ? paragraphbounds.data[idx + 1] : len;

		cv_clear(&buffer);

		rpfn(&nlstrippedbuf[start], end - start + 1, width, &buffer, num_indent_spaces, bAllowHyphenation);

		cv_strncat(output, buffer.data, buffer.length);
	}

	reflow_intstack_destroy(&paragraphbounds);
	cv_destroy(&buffer);
	free(nlstrippedbuf);
}

void ReflowText(const char* input, const size_t len, cv_t* output, struct ReflowParameters* parameters)
{
	ReflowTextImpl(input, len, output, parameters->line_width,
		parameters->num_indent_spaces, parameters->bAllowHyphenation, ReflowParagraph);
}

void ReflowTextBinary(const char* input, const size_t len, cv_t* output, struct ReflowParameters* parameters)
{
	ReflowTextImpl(input, len, output, parameters->line_width, parameters->num_indent_spaces,
		parameters->bAllowHyphenation, ReflowParagraphBinary);
}
