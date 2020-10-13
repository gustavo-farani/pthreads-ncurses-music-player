#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <list>
#include <vector>

class Song {
    std::string artist;
    std::string album;
    std::string title;
    int duration;
public:
    Song (
        const std::string& artist,
        const std::string& album,
        const std::string& title,
        int duration
    ) :
        artist(artist),
        album(album),
        title(title),
        duration(duration)
    {}
    // acessa a duração da música (em segundos)
    int getDuration () {
        return duration;
    }
    /* operator overloading que define a relação
     * de igualdade entre dois objetos Song */
    bool operator== (const Song& o) const {
        return artist == o.artist && album == o.album && title == o.title;
    }
    friend std::ostream& operator<< (std::ostream& os, const Song& o) {
        // TODO elaborar representação de objeto Song como string
        return os << o.title << std::endl;
    }
};

std::list<Song> playlist;
std::list<Song>::iterator currentlyPlaying;
bool userQuitted;

pthread_mutex_t mutex;
pthread_cond_t emptyPlaylist;

void addSong (const Song& song, int pos) {
    pthread_mutex_lock(&mutex);
    playlist.insert(std::next(playlist.begin(), pos), song);
    pthread_cond_signal(&emptyPlaylist);
    pthread_mutex_unlock(&mutex);
}

void removeSong (int pos) {
    pthread_mutex_lock(&mutex);
    playlist.erase(std::next(playlist.begin(), pos));
    pthread_mutex_unlock(&mutex);
}

bool playlistContainsSong (const Song& song) {
    for (const auto& it : playlist) {
        if (it == song) {
            return true;
        }
    }
    return false;
}

void quit () {
    pthread_mutex_lock(&mutex);
    userQuitted = true;
    pthread_mutex_unlock(&mutex);
}

Song getCurrentSong () {
    pthread_mutex_lock(&mutex);
    if (playlist.empty()) {
        // TODO programar uma user-defined exception apropriada
        pthread_mutex_unlock(&mutex);
        throw -1;
    } else {
        Song song = *currentlyPlaying;
        pthread_mutex_unlock(&mutex);
        return song;
    }
}

void* play (void* args) {
    while (true) {
        pthread_mutex_lock(&mutex);
        if (userQuitted) {
            pthread_mutex_unlock(&mutex);
            break;
        } else if (playlist.empty()) {
            pthread_cond_wait(&emptyPlaylist, &mutex);
            pthread_mutex_unlock(&mutex);
        } else {
            // comportamento cíclico
            if (currentlyPlaying == playlist.end()) {
                currentlyPlaying = playlist.begin();
            } else {
                currentlyPlaying++;
            }
            int duration = currentlyPlaying->getDuration();
            pthread_mutex_unlock(&mutex);
            sleep(duration);
        }
    }
    pthread_exit(NULL);
}

int main () {
    /* Por simplicidade, representamos as músicas
     * disponíveis dentro de um vetor na memória principal */
    std::vector<Song> database = {
        Song("Iron Maiden", "Fear of the Dark", "Fear of the Dark", 438),
        Song("The Beatles", "Abbey Road", "Here Comes the Sun", 185),
        Song("Pink Floyd", "Wish You Were Here", "Wish You Were Here", 317),
        Song("Elis Regina & Tom Jobim", "Elis & Tom", "Águas de Março", 212)
    };
    // inicializando explicitamente todas as variáveis globais
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&emptyPlaylist, NULL);
    userQuitted = false;
    currentlyPlaying = playlist.begin();

    // variáveis locais
    pthread_t playlistLoop;
    pthread_create(&playlistLoop, NULL, play, NULL);
    bool keep;

    do {
        // TODO imprimir opções de caracteres a serem digitados pelo usuário
        std::cout << "--opções--" << std::endl;

        // Lê o caractere que o usuário digitou
        char input;
        std::cin >> input;

        switch (input) {
            case '+': {
                // TODO interação com usuário para inserção de música
                int i;
                std::cin >> i;
                addSong(database[i], 0);
                keep = true;
            } break;
            case '-': {
                // TODO interação com usuário para remoção de música
                int i;
                std::cin >> i;
                removeSong(i);
                keep = true;
            } break;
            case '?': {
                try {
                    std::cout << getCurrentSong() << std::endl;
                } catch (...) {
                    std::cout << "A playlist está vazia" << std::endl;
                }
                keep = true;
            } break;
            case '.': {
                quit();
                std::cout << "O ciclo da playlist foi encerrado." << std::endl;
                std::cout << "Espere até a última música acabar." << std::endl;
                keep = false;
            } break;
            default: {
                std::cout << "Entrada inválida" << std::endl;
                keep = true;
            }
        }
    } while (keep);
    pthread_join(playlistLoop, NULL);
    // destruindo
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&emptyPlaylist);
}