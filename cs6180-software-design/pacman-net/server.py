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



class ServerGameSceneMain(engine.Scene):
    def __init__(self, screen, server_port):
        engine.Scene.__init__(self, screen)
        
        self.clock = pygame.time.Clock()
        self.wall_color_outside = 0, 0, 0 
        self.wall_color_middle = 50, 50, 50
        self.wall_color_inside = 100, 100, 100
        self.num_pellets = 60
        
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
        
        #Player info list
        players_info = [ {'name':'player_one', 'color':'red', 'position':(20,582)},\
                         {'name':'player_two', 'color':'blue','position':(582,20)}, \
                         {'name':'player_three','color':'green','position':(20,20)}, \
                         {'name':'player_four', 'color':'purple','position':(582,582)}, \
                         {'name':'player_five', 'color':'orange','position':(582,20)}, \
                         {'name':'player_six', 'color':'yellow','position':(582,20)}, \
                         {'name':'player_seven', 'color':'black','position':(582,20)}, \
                         {'name':'player_eight', 'color':'white','position':(582,20)} ]
        players_connected = 0
        self.clients = []

        #Set up server socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #server_socket.bind((socket.gethostname(), 55055))
        server_socket.bind(('localhost', server_port))
        print socket.gethostname()
        server_socket.listen(len(players_info))
        
        #Listen for register_time seconds, waiting for joining clients
        register_time = 10
        server_socket.settimeout(1)
        time_started = time_now = time.time()
        while time_now - time_started < 10:
            time_now = time.time()
            print "time left for clients to connect: %s" % \
                   (register_time - (time_now - time_started))
            #Accept client connection
            try:
                client = server_socket.accept()
                print "server: Got connection from %s on port %s" % client[1]
                #Tell client how much time till game start
                time_till_start = cPickle.dumps(register_time - (time_now - time_started))
                mysend(client[0], time_till_start)
            except socket.timeout:
                continue

            self.clients.append(client)
            #Get next client info
            info = players_info[players_connected]
            print "Sending client info %s" % info
            #Send client info
            mysend(client[0], cPickle.dumps(info))
            players_connected += 1
            if players_connected == len(players_info):
                break
        server_socket.settimeout(None)
        
        #Set up active player classes (Player, Walker)
        self.player_list = []
        self.walkers = []
        self.active_players = {}

        active_players_info = players_info[:players_connected]
        for p_info in active_players_info:
            p = engine.Player( p_info['name'] )
            w = Walker(p, p_info['position'], \
                       color=p_info['color'], speed=6)
            self.player_list.append(p)
            self.walkers.append(w)
            self.active_players[ p.name ] = (p, w)
            
        
        print 'Connected clients: %s' % self.clients
        #Send active player list to clients
        msg = cPickle.dumps(active_players_info)
        for client in self.clients:
            mysend(client[0], msg)


        #Scorekeeper and Scoreboard
        self.sk = engine.ScoreKeeper(self.player_list)
        self.sb = engine.ScoreBoard((170, self.screen.get_height()), 
                         self.sk, title="SCORES", color=(255,255,0))
        self.sb.rect = self.sb.get_rect().move(616, 0)

        
        #Pellets
        self.pellets = pygame.sprite.RenderUpdates()
        pellet_pos_taken = []
        pellet_positions = []
        for i in range(self.num_pellets):
            x = random.randrange(0,480,30)
            y = random.randrange(0,480,30)
            while (x,y) in pellet_pos_taken:
                x = random.randrange(0,480,30)
                y = random.randrange(0,480,30)
            pellet_pos_taken.append((x,y))
            x = int(x * (600.0/480.0)) + 4
            y = int(y * (600.0/480.0)) + 4
            pellet_positions.append((x,y))
            self.pellets.add(Pellet((x,y)))
        
        #Send active player list to clients
        msg = cPickle.dumps(pellet_positions)
        for client in self.clients:
            mysend(client[0], msg)
        
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
        while 1:
            #self.clock.tick(30)

            dirty_rects = []
            walker_rects = {}
            walker_info = {}

            for client in self.clients:
                #Recieve client info
                client_info = cPickle.loads(myrecv(client[0]))
                client_name = client_info['name']
                p, w = self.active_players[client_name]
                w.direction = client_info['direction']
                w.speed = client_info['speed']

                dirty_rects.append(w.rect)
                self.screen.blit(self.background, w.rect, w.rect)
                #Move client
                w.rect = client_info['rect']
        
            #Draw pellets
            dirty_rects.extend(self.pellets.draw(self.screen))

            for walker in self.walkers:
                #Check pellet collisions
                pellet_collides = pygame.sprite.spritecollide(walker, self.pellets, 1)
        
                if len(pellet_collides) > 0:
                    walker.lightup()
                    for p in pellet_collides:
                        self.screen.blit(self.background, p.rect, p.rect)
                    score = self.sk.get_score(walker.player) + len(pellet_collides)
                    self.sk.set_score(walker.player, score)
                    #print 'Player %s has gotten %d pellets!' % (walker.player.name, score)
        
                    if(len(self.pellets) <= 0):
                        return
        
                    #Increase speed based on score
                    if(score >= 8 and score < 32):
                        walker.speed = 8
                    elif(score >= 32 and score < 64):
                        walker.speed = 10
                    elif(score >= 64):
                        walker.speed = 12

        
                self.screen.blit(walker.image, walker.rect)
                walker_rects[walker.player.name] = walker.rect

            #Draw scoreboard
            self.sb.update()
            self.screen.blit(self.sb, self.sb.rect)
            dirty_rects.append(self.sb.rect)

            #Add in walker rects to dirty_rects
            for walker_rect in walker_rects.values():
                dirty_rects.append(walker_rect)

            #Finally do dirty rectangle update
            pygame.display.update(dirty_rects)

            #Send all walker updates to clients
            for client in self.clients:
                mysend(client[0], cPickle.dumps(walker_rects))


screen_size = width, height = (800, 600)
screen = pygame.display.set_mode(screen_size)
screen.fill((0,0,0))

gsm = ServerGameSceneMain(screen, server_port=55055)
gsm.runloop()

print 'Thanks for playing!'
