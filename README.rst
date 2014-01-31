=================
UFS-910 Imageutil
=================

What is this all about?
=======================

The imageutil is a small c program which is capable of reading and writing
images to the flashrom of the `Kathrein UFS-910 satellite receiver`__ using its
network infterface. The only prerequisite needed for this to work is a running
telnet daemon on the receiver and a working ethernet connection with it.

__ http://www.kathrein.de/servicede/produktsuche.cfm?id=699&sprache=gb

The tool has been tested on different kinds of images and should therefore
work under nearly all conditions. It should anyhow considered to be highly
experimental and used with caution. I do not take any responsibility if eats
your cat hides your socks or fries your reciever.


How do I use the tool?
======================

Imageutil is a shell application written for linux based systems. Just call
the util by it's name in a console and it will give you it's usage information
containing example on how the different functions work. ::

	$ ./imageutil

	Kathrein UFS-910 Image Utillity v. 0.2
	(c) Jakob Westhoff <jakob@westhoffswelt.de>
	Usage: ./imageutil ACTION <ARGUMENTS ...>
	Everything your ufs910 images need.

	Possible actions:
	  r: Read all images from the receiver and store them on the pc.
		 Parameters: [kathreinip] [targetpath]
		 Example: ./imageutil r 10.0.1.202 targetpath

	  w: Write all images from your pc to the receiver.
		 WARNING: THIS FUNCTION IS HIGHLY EXPERIMENTAL AND MAYBE DANGEROUS
		 Parameters: [kathreinip] [sourcepath]
		 Example: ./imageutil w 10.0.1.202 sourcepath
	 
	  h: Add header and checksum to an image.
		 Parameters: [imagetype] [source image] [destination image]
		 Valid imagetypes: kernel, config, root, app, emergency, data, bootcfg
		 Example: ./imageutil h app image_without_header.img image_with_header.img

	  s: Strip header and checksum from image.
		 Parameters: [source image] [destination image]
		 Example: ./imageutil s image_with_header.img image_without_header.img

