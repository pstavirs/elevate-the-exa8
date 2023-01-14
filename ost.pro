TEMPLATE = subdirs
SUBDIRS = client ostproto ostprotogui rpc extra cfgagent

client.target = client
client.file = client/ostinato.pro
client.depends = ostproto ostprotogui rpc extra cfgagent

server.target = server
server.file = server/drone.pro
server.depends = ostproto rpc

ostproto.file = common/ostproto.pro

ostprotogui.file = common/ostprotogui.pro
ostprotogui.depends = extra

rpc.file = rpc/pbrpc.pro

cfgagent.file = cfgagent/cfgagent.pro
cfgagent.depends = ostproto
