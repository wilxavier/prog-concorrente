// William Xavier dos Santos (19/0075384)

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define N 3     // quantidade de grupos e de elementos no grupo
#define L 30    // quantidade de leite produzido pela vaca
#define V 1     // quantidade de vendedores de queijo
#define F 1     // quantidade de fazendeiros
#define LQ 15   // quantidade de leite necessário para fazer 1 queijo

int leite[N] = {0};    // array para contar o volume de leite cada vaca
int bQuer = 0;         // variável para indicar a preferência do bezerro

pthread_t vacas[N];            // threads vacas
pthread_t bezerros[N];         // threads bezerros
pthread_t funcionarios[N];     // threads funcionários
pthread_t vendedor[V];         // thread vendedor

pthread_cond_t vaca_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t bezerro_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t funcionario_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t teta = PTHREAD_MUTEX_INITIALIZER;

sem_t queijos;

void *vaca(void *arg);
void *bezerro(void *arg);
void *funcionario(void *arg);
void *venda(void *arg);

int main(int argc, char**argv) {
    int i;
    int *id;
    
    sem_init(&queijos, 0, 0);        // semáforo para queijos

    // deploy threads vacas
    for (i = 0; i < N; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        pthread_create(&vacas[i], NULL, vaca, (void *)(id));
    }

    // deploy threads bezerros
    for (i = 0; i < N; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        pthread_create(&bezerros[i], NULL, bezerro, (void *)(id));
    }

    // deploy threads funcionários
    for (i = 0; i < N; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        pthread_create(&funcionarios[i], NULL, funcionario, (void *)(id));
    }

    // deploy threads vendedor
    for (i = 0; i < V; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        pthread_create(&vendedor[i], NULL, venda, (void *)(id));
    }

    pthread_join(vacas[0], NULL);
   

    sem_destroy(&queijos);       // destuíndo semáforo de queijos
    return 0;
}

void *vaca(void *arg) {
    int id = *((int *) arg);    // id da vaca
    int grupo = id%N;           // identificador do grupo
    printf("\tGrupo %d - Vaca %d criada\n", grupo, id);

    while(1) {
        pthread_mutex_lock(&teta); // vaca  pega lock teta
            while(leite[grupo] > 5) {      // enquanto houver leite
                pthread_cond_wait(&vaca_cond, &teta);  // adormece vaca
            }
            printf("-- Vaca %d pastando\n", id);
            leite[grupo] = L;   // enche quantidade de leite do grupo
            sleep(2);

            printf("Grupo %d - Vaca %d finalizou a pastagem\t\t\tTotal de leite no grupo: %d\n", grupo, id, leite[grupo]);
            pthread_cond_signal(&bezerro_cond); // acorda bezerro 
            pthread_cond_signal(&funcionario_cond); // acorda funcionário
        pthread_mutex_unlock(&teta);   // vaca solta lock da teta
    }
    pthread_exit(0);
}

void *bezerro(void *arg) {
    int id = *((int *) arg);    // id do bezerro
    int grupo = id%N;           // identificador do grupo
    int leite_bebido = 0;       // contador de todo o leite que foi mamado
    printf("\tGrupo %d - Bezerro %d criado \n", grupo, id);

    while(1) {
        sleep(5+rand()%5);
        pthread_mutex_lock(&teta); // bezerro pega lock da teta
            bQuer++;    // incrementa variável para indicar que o bezerro quer mamar
            while(leite[grupo] < 10) {  // enquanto qnt de leite do grupo for menor que a qnt que o bezerro bebe 
                pthread_cond_signal(&vaca_cond);            // acorda vaca
                pthread_cond_wait(&bezerro_cond, &teta);   // adormece bezerro
                printf("\tO suprimento de leite está comprometido! Bezerro %d quer mamar!\n", id);
            }
            bQuer--;    // decrementa variável para indicar que o bezerro quer mamar
            leite[grupo] = leite[grupo] - 10;   // decrementa quantidade de leite do grupo
            leite_bebido = leite_bebido + 10;   // incrementa a quantidade total de leite bebido
            printf("Grupo %d - Bezerro %d acabou de mamar\t\t\tLeite restante no grupo: %d\t\tTotal de leite consumido: %d\n", grupo,id, leite[grupo], leite_bebido);
        pthread_mutex_unlock(&teta);   // bezerro solta o lock da teta
    }
    pthread_exit(0);
}

void *funcionario(void *arg) {
    int id = *((int *) arg);    // id funcionario
    int grupo = id%N;           // identificador grupo
    int leite_retirado = 0;     // contador de todo o leite ordenhado
    int value = 0;
    printf("\tGrupo %d - Funcionario %d criado \n", grupo, id);

    while(1) {
        sleep(rand()%3);
        pthread_mutex_lock(&teta); // funcionário pega lock da teta
            while(leite[grupo] < 5 || bQuer > 0 ) { // enquanto qnt de leite for insuficiente ou bezerro quiser beber
                pthread_cond_signal(&vaca_cond);            // acorda vaca
                pthread_cond_wait(&funcionario_cond, &teta);   // adormece bezerro
                if (leite[grupo] < 5) printf("\tO suprimento de leite está comprometido! Funcionário %d precisa ordenhar!\n", id);
                if (bQuer > 0) printf("\tBezerro %d quer mamar! Funcionário %d terá que esperar!\n", id, id);
            }

            leite[grupo] = leite[grupo] - 5;   // decrementa quantidade de leite do grupo
            leite_retirado = leite_retirado + 5;   // incrementa a quantidade total de leite retirado
            printf("Grupo %d - Funcionário %d acabou de fazer a ordenha\t\tLeite restante no grupo: %d\t\tTotal de leite retirado: %d\n", grupo, id, leite[grupo], leite_retirado);
        pthread_mutex_unlock(&teta); // funcionário solta lock da teta
        while (leite_retirado >= LQ) {  // enquanto tiver leite retirado suficiente para fazer queijo
            sem_post(&queijos); // incrementa semáforo de queijos
            leite_retirado = leite_retirado - LQ;   // decrementa quantidade de leite retirado
            sem_getvalue(&queijos, &value); // pega valor do semáforo para saber quantos queijos existem
            printf("* Grupo %d - Produzindo QUEIJO!\t\t\t\tLeite restante no grupo: %d\t\tTotal de queijo: %d\n", grupo, leite[grupo], value);
        }
    }
    pthread_exit(0);
}

void *venda(void *arg) {
    int queijos_vendidos = 0;   // contador de queijos vendidos
    int value = 0;
    printf("\tVendedor criado! \n");

    while(1) {
        sleep(2+rand()%5);
        sem_wait(&queijos); // decrementa semáforo de queijo
        queijos_vendidos++; // incrementa contador de queijos vendidos
        sem_getvalue(&queijos, &value); // pega valor do semáforo para saber quantos queijos existem
        printf("* VENDA EXECUTADA!\t\t\t\t\t\tQueijos restantes: %d\t\tTotal de queijos vendidos: %d\n", value, queijos_vendidos);
    }
    pthread_exit(0);
}