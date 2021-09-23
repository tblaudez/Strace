# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: tblaudez <tblaudez@student.codam.nl>         +#+                      #
#                                                    +#+                       #
#    Created: 2021/03/31 15:06:14 by tblaudez      #+#    #+#                  #
#    Updated: 2021/05/12 10:06:26 by tblaudez      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

TARGET := ft_strace
CFLAGS := -Wall -Wextra -Werror -I include/ $(EXTRA_FLAGS)

SOURCES := $(shell find src/ -name "*.c")
HEADERS := $(shell find include/ -name "*.h")
OBJECTS := $(SOURCES:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fv $(OBJECTS)

fclean: clean
	@rm -fv $(TARGET)

re: fclean all

.PHONY: all clean fclean re