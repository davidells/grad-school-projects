#!/usr/bin/env python

# Written by David Ells
#
# A pacman-like game written using the pygame package.
# Originally written as project 1 for Dr. Butler's 
# Software Design and Development class (cs6180)

import pygame, random, sys
import engine
pygame.init()

class Walker(engine.Movable_2D, engine.AnimatedSprite):
    def __init__(self, player, startpos):
        engine.Movable_2D.__init__(self)
        engine.AnimatedSprite.__init__(self)

        self.player = player

        self._set_static_image(pygame.image.load("square_red.gif").convert())
        self.__light_animation = self._get_light_animation()

        self.rect = self.image.get_rect()
        self.rect.center = startpos

    def _get_light_animation(self):
        light_image = pygame.image.load("square_red_lit.gif").convert()
        return [light_image for i in range(20)]

    def lightup(self):
        self.animation = self.__light_animation

    def update(self):
        self.rect = self.rect.move(self.velocity)


class Pellet(pygame.sprite.Sprite):
    def __init__(self, startpos):
        pygame.sprite.Sprite.__init__(self)
        self.image = pygame.image.load("cube.gif").convert()
        self.image.set_colorkey((255,255,255), pygame.RLEACCEL)
        self.rect = self.image.get_rect()
        self.rect.topleft = startpos


class Wall(pygame.sprite.Sprite):
    def __init__(self):
        pygame.sprite.Sprite.__init__(self)



class GameSceneMain(engine.Scene):
    def __init__(self, screen):
        engine.Scene.__init__(self, screen)
        
        self.clock = pygame.time.Clock()
        self.wall_color_outside = 0, 0, 0 
        self.wall_color_middle = 50, 50, 50
        self.wall_color_inside = 100, 100, 100
        self.num_pellets = 20
        
        #background = pygame.image.load("stars.jpg").convert()
        self.background = pygame.Surface(self.screen.get_size()).convert()
        self.background.fill((50,50,50))
        
        #Lay some tile...
        self.tile = pygame.image.load("cleantile.gif").convert()
        for i in range(0, 480, 30):
            for j in range(0, 480, 30):
                rect = self.tile.get_rect()
                rect.top = int(i*(600.0/480.0))
                rect.left = int(j*(600.0/480.0))
                self.background.blit(self.tile, rect)
        
        #Player
        self.p1 = engine.Player('player_one')
        self.p2 = engine.Player('player_two')
        
        #Scorekeeper and Scoreboard
        self.sk = engine.ScoreKeeper([self.p1, self.p2])
        self.sb = engine.ScoreBoard((170, self.screen.get_height()), 
                         self.sk, title="SCORES", color=(255,255,0))
        self.sb.rect = self.sb.get_rect().move(616, 0)
        
        #Walker
        self.w = Walker(self.p1, (20,582))
        self.w.speed = 2
        
        #Pellets
        self.pellets = pygame.sprite.RenderUpdates()
        pellet_pos_taken = []
        for i in range(self.num_pellets):
            x = random.randrange(0,480,30)
            y = random.randrange(0,480,30)
            while (x,y) in pellet_pos_taken:
                x = random.randrange(0,480,30)
                y = random.randrange(0,480,30)
            pellet_pos_taken.append((x,y))
            x = int(x * (600.0/480.0)) + 4
            y = int(y * (600.0/480.0)) + 4
            self.pellets.add(Pellet((x,y)))
        
        
        #Walls
        self.walls = pygame.sprite.Group()
        
        #Read in maze file
        f = open("board.txt")
        lines = f.readlines()
        for line in lines:
            #If not a comment line
            if not line[0] == "#":
                x1, y1, x2, y2 = map(lambda x: int(x)*(600.0/480.0), line.split(' '))
                temprect = (pygame.draw.line (self.background, self.wall_color_outside,
                                              (x1, y1), (x2, y2), 8))
                temprect.inflate_ip(-4, -4)
                pygame.draw.line (self.background, self.wall_color_middle, 
                                  (x1, y1), (x2, y2), 4)
                pygame.draw.line (self.background, self.wall_color_inside, 
                                  (x1, y1), (x2, y2), 1)
        
                #Create wall with this area
                wall = Wall()
                wall.rect = temprect
                self.walls.add(wall)
        

    def runloop(self):
        #One time full screen draw
        self.screen.blit(self.background, self.background.get_rect())
        pygame.display.update()
        
        #Game loop
        dirty_rects = []
        while 1:
            self.clock.tick(60)
            for event in pygame.event.get():
                if event.type == pygame.QUIT: 
                    sys.exit()
        
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        return
                    if event.key == pygame.K_DOWN:
                        self.w.start_move_down()
                    elif event.key == pygame.K_UP:
                        self.w.start_move_up()
                    elif event.key == pygame.K_RIGHT:
                        self.w.start_move_right()
                    elif event.key == pygame.K_LEFT:
                        self.w.start_move_left()
                    elif event.key == pygame.K_a:
                        self.w.slowdown = True
        
                elif event.type == pygame.KEYUP:
                    if event.key == pygame.K_DOWN and self.w.is_moving_down():
                        self.w.stop_move_down()
                    elif event.key == pygame.K_UP and self.w.is_moving_up():
                        self.w.stop_move_up()
                    elif event.key == pygame.K_RIGHT and self.w.is_moving_right():
                        self.w.stop_move_right()
                    elif event.key == pygame.K_LEFT and self.w.is_moving_left():
                        self.w.stop_move_left()
                    elif event.key == pygame.K_a:
                        self.w.slowdown = False
        

            #Draw pellets
            dirty_rects.extend(self.pellets.draw(self.screen))

            #Go ahead and move, then test if move is valid
            dirty_rects.append(self.w.rect)
            self.screen.blit(self.background, self.w.rect, self.w.rect)
            self.w.rect = self.w.rect.move(self.w.velocity)

            #Prevent sprite from moving through walls, stopping sprite if needed
            wall_collides = pygame.sprite.spritecollide(self.w, self.walls, 0)
            if len(wall_collides) > 0:
                for wall in wall_collides:
                    #Moving right
                    if self.w.is_moving_right():
                        overlap = self.w.rect.right - wall.rect.left
                        if 0 < overlap and overlap <= self.w.speed:
                            self.w.rect.right = wall.rect.left - 1
                    #Moving left
                    if self.w.is_moving_left():
                        overlap = wall.rect.right - self.w.rect.left
                        if 0 < overlap and overlap <= self.w.speed:
                            self.w.rect.left = wall.rect.right + 1
                    #Moving down
                    if self.w.is_moving_down():
                        overlap = self.w.rect.bottom - wall.rect.top
                        if 0 < overlap and overlap <= self.w.speed:
                            self.w.rect.bottom = wall.rect.top - 1
                    #Moving up
                    if self.w.is_moving_up():
                        overlap = wall.rect.bottom - self.w.rect.top
                        if 0 < overlap and overlap <= self.w.speed:
                            self.w.rect.top = wall.rect.bottom + 1

                self.w.stop_moving()
                #print 'Hit a wall'


            #Check pellet collisions
            pellet_collides = pygame.sprite.spritecollide(self.w, self.pellets, 1)

            if len(pellet_collides) > 0:
                self.w.lightup()
                for p in pellet_collides:
                    self.screen.blit(self.background, p.rect, p.rect)
                score = self.sk.get_score(self.w.player) + len(pellet_collides)
                self.sk.set_score(self.w.player, score)
                #print 'Player %s has gotten %d pellets!' % (self.w.player.name, score)

                if(len(self.pellets) <= 0):
                    return

                #Increase speed based on score
                if(score >= 8 and score < 32):
                    self.w.speed = 4
                elif(score >= 32 and score < 64):
                    self.w.speed = 6
                elif(score >= 64):
                    self.w.speed = 8


            self.screen.blit(self.w.image, self.w.rect)
            dirty_rects.append(self.w.rect)

            #Draw scoreboard
            self.sb.update()
            self.screen.blit(self.sb, self.sb.rect)
            dirty_rects.append(self.sb.rect)

            #Finally do dirty rectangle update and reset dirty_rects
            pygame.display.update(dirty_rects)
            dirty_rects = []


screen_size = width, height = (800, 600)
screen = pygame.display.set_mode(screen_size)
#screen = pygame.display.set_mode(screen_size, pygame.FULLSCREEN)
screen.fill((0,0,0))

gsm = GameSceneMain(screen)
gsm.runloop()

print 'Thanks for playing!'
