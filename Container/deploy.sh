# this scirpt build the executable from the file and move into docker and then execute

# remove executable if exists
[ -e SNR_CONTAINER ] && rm SNR_CONTAINER

# build executable
make container

# No need for container since we are on our own server
# move executable into container 427docker
docker cp SNR_CONTAINER 427docker:/home

# execute docker
docker exec -it 427docker /bin/bash

sudo ./SNR_CONTAINER -u 1 -m jrootfs -c /bin/bash -C 2 -s 0 -p 15 -M 104857600 -r "7:1 10485760" -w "7:1 10485760" -H group47