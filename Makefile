# Nom de l'exécutable final
EXEC = programme

# Compilateur
CC = gcc

# Options de compilation (ajoute -g pour le debug ou -Wall pour les warnings)
CFLAGS = -Wall -Wextra -std=c11

# Tous les fichiers .c du dossier courant
SRCS = $(wildcard *.c)

# Transformation des .c en .o
OBJS = $(SRCS:.c=.o)

# Règle par défaut
all: $(EXEC)

# Linkage
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation des .c en .o (avec dépendances automatiques)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f *.o *.d $(EXEC)

# Nettoyage complet
fclean: clean

# Recompilation complète
re: fclean all

.PHONY: all clean fclean re
