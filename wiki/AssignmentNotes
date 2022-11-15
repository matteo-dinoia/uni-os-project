## Assignment tasks and deadline

* The source code (.zip or .tgz)
* A brief report about project implementation choices
* To deliver not after **14/01/2023 at 00:00**
* Must **NOT** be published before **30/11/2023**


## Implementation requirements

* Must be divided in different modules (for example, one for each different process capable of being launched with ```execve(…)```)
* Must use the ```make``` utility
* Must be compiled with at least ```gcc -std=c89 -pedantic```
* Must maximize process concurrency, whenever possible 
* Must free all resource at process termination
* Must define the ```_GNU_SOURCE``` macro


### Minimal project 

#### Processes
* A *master* process, that will create and manage other processes 
* A ```SO_NAVI``` number of *ship* processes
* A ```SO_PORTI``` number of *port* processes

#### Data 
* goods (goods_type_id in [0, SO_MERCI-1], size in [1, SO_SIZE], shelf_life in [SO_MIN_VITA, SO_MAX_VITA])
  * shelf life in days
  * when shelf life became 0 it disappears (anywhere it is stored)
* ship (speed = SO_SPEED (km/day), double position[2], max_capacity=SO_CAPACITY)
* port (double position[2], num_docks)
* map is a square of side SO_LATO (flat earth -> line navigation)
  * 0<=position[i]<=SO_LATO

#### Functionality
* ship automatically move around
  * ship movement is made with nanosleep(distance/velocity) 
  * when at port can load or unload 
    * load/unload time is based on quantity of goods moved 
* port
  * when a ship arrive at docks ask to access docks for load and unload (with semaphore)
  * they generate offer and request randomly


### Full project
#### Processes
* meteo manage metereological condititions for ports and ships. It can create: 
  

#### Functionality
* meteo
  * Tempest: change speed of ships
  * Sea_storm: stop random port
  * maelstrom: sink some ship


