Some source code and additional files for https://starbasesim.com

The StarbaseSimLibrary folder contains the C++ files from the Unreal Project. I don't know, if it will be of much use to anybody. Some parts might be hard to follow as half of the code is in the Blueprints, which are not included here. But other things like the tank farm and position controller are mostly C++.  
You could probably use AI to set up a quick sample project to make use of the position controller code. Then maybe you could improve on it and send me your improvements and it would work in my project. But that's a big maybe.
In the end this was just one reason to release the source code, the idea that someone could help me like this. The other reason is that this could be the start to making an open source project out of StarbaseSim. Even though this should not be done in Unreal Engine imho. But we'll see how much time and motivation I have going forward. At least, with this release here, somebody else could have a little bit of a starting point.

Some notes: Most of the C++ code has been hand written over the course of four years as some of it, like the tank farm code is mostly just a conversion from the old C# Unity code from 2022. Only towards the end, I used more and more AI. The trajectory prediction stuff was mostly done by Gemini for example. But most of the code has been created before 2025.

With AI it should be possible to do much better code, much quicker nowadays. So in a project like StarbaseSim most of the work will go into 3d models, UI, design, optimzation and such things. I can't share my models right now. But I will look into it.
