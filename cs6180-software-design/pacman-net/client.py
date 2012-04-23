#!/usr/bin/env python

# Written by David Ells
#
# A pacman-like game written using the pygame package.
# Originally written as project 1 for Dr. Butler's 
# Software Design and Development class (cs6180)

import pygame, random, sys, socket, struct, time
import cPickle
import engine
pygame.init()


int_size = struct.calcsize("i")

def mysend(s, msg):
    s.send(struct.pack("i",len(msg)))
    s.send(msg)

def myrecv(s):
    msg_len = struct.unpack("i", s.recv(int_size))[0]
    return s.recv(msg_len)
    


class Walker(engine.Movable_2D, engine.AnimatedSprite):
    def __init__(self, player, startpos, color, speed=2):
        engine.Movable_2D.__init__(self)
        engine.AnimatedSprite.__init__(self)

        self.player = player

        self.image_name = "square_%s.gif" % color
        self.lit_image_name = "square_%s_lit.gif" % color

        self._set_static_image(pygame.image.load(self.image_name).convert())
        self.__light_animation = self._get_light_animation()

        self.speed = speed
        self.rect = self.image.get_rect()
        self.rect.center = startpos

    def _get_light_animation(self):
        light_image = pygame.image.load(self.lit_image_name).convert()
        return [light_image for i in range(10)]

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



class ClientGameSceneMain(engine.Scene):
    def __init__(self, screen, server_hostname, server_port):
        engine.Scene.__init__(self, screen)
        
        self.clock = pygame.time.Clock()
        self.wall_color_outside = 0, 0, 0 
        self.wall_color_middle = 50, 50, 50
        self.wall_color_inside = 100, 100, 100
        
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
        
        #Set up socket, contact server, and get client info
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((server_hostname, server_port))
        print "client: Successfully connected"
        time_till_game = cPickle.loads(myrecv(self.socket))
        print "Time until game starts: %s" % time_till_game
        print "Requesting client info..."
        self.client_info = cPickle.loads(myrecv(self.socket))
        self.active_players_info = cPickle.loads(myrecv(self.socket))
        print self.client_info
        print self.active_players_info

        #Set up active player classes (Player, Walker)
        self.player_list = []
        self.walkers = []
        self.active_players = {}

        for p_info in self.active_players_info:
            p = engine.Player( p_info['name'] )
            w = Walker(p, p_info['position'], \
                       color=p_info['color'], speed=4)
            self.player_list.append(p)
            self.walkers.append(w)
            self.active_players[ p.name ] = (p, w)

            #Set special reference to my walker
            if p_info['name'] == self.client_info['name']:
                self.w = w


        #Scorekeeper and Scoreboard
        self.sk = engine.ScoreKeeper(self.player_list)
        self.sb = engine.ScoreBoard((170, self.screen.get_height()), 
                         self.sk, title="SCORES", color=(255,255,0))
        self.sb.rect = self.sb.get_rect().move(616, 0)
        
        #Recieve pellet locations from server
        pellet_locations = cPickle.loads( myrecv(self.socket) )

        #Pellets
        self.pellets = pygame.sprite.RenderUpdates()
        for p_loc in pellet_locations:
            self.pellets.add(Pellet(p_loc))


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
        score = 0

        #Game loop
        while 1:
            self.clock.tick(60)

            dirty_rects = []
            walker_info = {}

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


            #Try to move, then check if valid
            self.screen.blit(self.background, self.w.rect, self.w.rect)
            dirty_rects.append(self.w.rect)
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

            #Send my info to server
            walker_info['name'] = self.w.player.name
            walker_info['direction'] = self.w.direction
            walker_info['speed'] = self.w.speed
            walker_info['rect'] = self.w.rect
            mysend(self.socket, cPickle.dumps(walker_info))

            #Get walker updates
            walker_rects = cPickle.loads( myrecv(self.socket) )
            for player_name in walker_rects.keys():
                p, w = self.active_players[player_name]
                #Draw background over old walker position
                dirty_rects.append(w.rect)
                self.screen.blit(self.background, w.rect, w.rect)
                #Update walker position and draw it's image on the screen
                w.rect = walker_rects[player_name]
                self.screen.blit(w.image, w.rect)
                dirty_rects.append(w.rect)
                
            #Draw pellets
            dirty_rects.extend(self.pellets.draw(self.screen))

            #Check pellet collisions
            for player_name in walker_rects.keys():
                p, w = self.active_players[player_name]
                pellet_collides = pygame.sprite.spritecollide(w, self.pellets, 1)
                if len(pellet_collides) > 0:
                    w.lightup()
                    for pel in pellet_collides:
                        self.screen.blit(self.background, pel.rect, pel.rect)

                    score = self.sk.get_score(p) + len(pellet_collides)
                    print 'player %s has score %s' % (p.name, score)
                    self.sk.set_score(p, score)

                    if w == self.w:
                        #Increase speed based on score
                        if(score >= 8 and score < 32):
                            self.w.speed = 6
                        elif(score >= 32 and score < 64):
                            self.w.speed = 8
                        elif(score >= 64):
                            self.w.speed = 10

            #Draw scoreboard
            self.sb.update()
            self.screen.blit(self.sb, self.sb.rect)
            dirty_rects.append(self.sb.rect)

            #Finally do dirty rectangle update and reset dirty_rects
            pygame.display.update(dirty_rects)

    

screen_size = width, height = (800, 600)
screen = pygame.display.set_mode(screen_size)
screen.fill((0,0,0))

gsm = ClientGameSceneMain(screen, server_hostname="localhost", server_port=55055)
gsm.runloop()

print 'Thanks for playing!'
