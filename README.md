This is a tool to make calculations for reporting the liabilities and assets for Belgian DC pension plans under IAS19. It is still in progress, but if you really want to try it then you will need to run make in the folders lib/actuarial, lib/general, lib/vba and lib/excel. If you run accross some compiling issues then it is most likely due to uninstalled libraries. Follow the errors to see if you can install them. You can also read the make files as they list dependencies. I haven't took the time yet to automate this process.
After the libraries compile you can run make in the DC folder and if successful run ./main. For now you will just get an interface, but to actually run you will need an excel file in the correct format. I will provide an example in some later stages.

The more interesting libraries for now are the libraries in the folder lib. They define some useful functions and macros to use, most notably lib/general that defines the stretchy buffer functionality, an arena allocator and a simple hash map.
In lib/excel you can compile the functions neccessary to read an excel file,
with functionality of reading a value in a given cell or reading a database with
column names as keys.
lib/vba was designed to interpret VBA (Visual Basic for Applications), with
some useful added functionality like reading a value from an excel database.
If you're familiar with actuarial science then you may like lib/actuarial,
which defines the most famous actuarial functions.
Feel free to poke around, but I warn you that the repositories haven't been made with other users in mind.
