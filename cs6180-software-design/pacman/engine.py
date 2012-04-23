# Written by David Ells
#
# A module containing useful classes for creating games using pygame

import pygame, random, sys

class Player(object):
    def __init__(self, name):
        self.name = name
        self.score = 0

class Movable_2D(object):
    def __init__(self):
        self._speed = 1
        self._direction = [0, 0]
        self.slowdown = False

    def _get_speed(self):
        return self.slowdown and 1 or self._speed
    def _set_speed(self, val):
        self._speed = val
    speed = property(_get_speed, _set_speed)

    def _get_direction(self):
        return self._direction
    def _set_direction(self, val):
        self._direction = val
    direction = property(_get_direction, _set_direction)
    
    def _get_velocity(self):
        return [self.speed * self.direction[0],\
                self.speed * self.direction[1]]
    velocity = property(_get_velocity)

    def start_move_up(self):
        self._direction[1] = -1
    def stop_move_up(self):
        self._direction[1] = 0
    def is_moving_up(self):
        return self._direction[1] == -1 

    def start_move_right(self):
        self._direction[0] = 1
    def stop_move_right(self):
        self._direction[0] = 0
    def is_moving_right(self):
        return self._direction[0] == 1

    def start_move_down(self):
        self._direction[1] = 1
    def stop_move_down(self):
        self._direction[1] = 0
    def is_moving_down(self):
        return self._direction[1] == 1

    def start_move_left(self):
        self._direction[0] = -1
    def stop_move_left(self):
        self._direction[0] = 0
    def is_moving_left(self):
        return self._direction[0] == -1

    def stop_moving(self):
        self._direction = [0, 0]


class AnimatedSprite(pygame.sprite.Sprite):
    def __init__(self):
        pygame.sprite.Sprite.__init__(self)
        self.__static_image = None
        self.__animation = None
        self.__animation_frames_left = 0

    def _get_animation(self):
        return self.__animation
    def _set_animation(self, imagearray):
        self.__animation_frames_left = len(imagearray)
        self.__animation = imagearray
    animation = property(_get_animation, _set_animation)

    def _get_image(self):
        if self.__animation_frames_left > 0:
            self.__animation_frames_left -= 1
            return self.__animation[self.__animation_frames_left]
        else:
            return self.__static_image
    image = property(_get_image)

    def _set_static_image(self, val):
        self.__static_image = val


class ScoreKeeper(object):
    def __init__(self, playerlist):
        self.__players = playerlist
        self.__scores = {}
        for p in self.__players:
            self.__scores[p] = 0

    def get_score(self, player):
        if player in self.__players:
            return self.__scores[player]
        else:
            return 0

    def set_score(self, player, newscore):
        if player in self.__players:
            self.__scores[player] = newscore

    def get_players(self):
        return self.__players


class ScoreBoard(pygame.Surface):
    def __init__(self, (width, height), scorekeeper, font=None, \
                 color=(255,255,0), title="Scoreboard", spacing=10, \
                 topmargin=10, leftmargin=10):
        pygame.Surface.__init__(self, (width, height))
        self.__scorekeeper = scorekeeper
        self.__background = None
        if font is None:
            self.__font = pygame.font.SysFont("Comic Sans MS", 18)
        else:
            self.__font = font
        self.__color = color
        self.__title = title
        self.__spacing = spacing
        self.__topmargin = 10
        self.__leftmargin = 10

    def _get_user_labels(self):
        labels = []
        for p in self.__scorekeeper.get_players():
            labels.append(self.__font.render(self._get_player_entry(p),\
                                        1, self.__color ))
        return labels

    def _get_title_label(self):
        return self.__font.render(self.__title, 2, self.__color)

    def _get_player_entry(self, player):
        sk = self.__scorekeeper
        return "%s : %s" % (player.name, sk.get_score(player))

    def set_background(self, bg):
        self.__background = bg

    def set_color(color):
        self.__color = color

    def update(self):
        if self.__background is None:
            self.fill((0,0,0))
        else:
            self.blit(self.__background, self.get_rect())

        title = self._get_title_label()
        self.blit(title, title.get_rect().move(self.__leftmargin, 
                                               self.__topmargin))
        line_offset = self.__topmargin + title.get_height()
        pygame.draw.line(self, 
                         self.__color,
                         (10, line_offset), 
                         (self.get_width() - 10, line_offset), 
                         2)
    
        labels = self._get_user_labels()
        label_offset = line_offset + self.__spacing
        for label in labels:
            self.blit(label, label.get_rect().move(self.__leftmargin, 
                                                   label_offset))
            label_offset += (label.get_height() + self.__spacing)


class Scene(object):
    def __init__(self, screen):
        self.screen = screen
    def runloop(self):
        raise Exception("Abstract method called!")
