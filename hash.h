#pragma once
#include <string>

int hash(std::string s)
{
    long long sum = 0;
    int div = pow(2, 16) - 1;
    int size = s.size();
    const char* buf = s.c_str();
    char c;
    int vett[] = { 17,349,1153,2081,3361,4517,5573,6733,7411,16127,37199,93911,199933 };
    int n = s.c_str()[size / 2] % 13;
    for (int i = 0; i < size; i++)
    {
        c = buf[i];
        sum += sum * vett[n] + c * vett[(n * size) % 13] + (n + size % (i + 1)) * vett[c % 13];
        n = (n + c + sum) % 13;
        sum = sum * 999983 + c * 37199; //totalmente a caso, sono 2 numeri primi random che danno una buona distribuzione dei risultati
    }
    sum = abs(sum); // voglio solo
    sum = sum % div;// numeri tra 0 e 2^16-1 perchè un int può avere dimensione variabile ma è di almeno 16 bit
    return int(sum);
}