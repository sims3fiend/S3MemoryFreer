# S3MemoryFreer
Simple little .asi thing to (currently) send you a message when you're close to hitting the sims 3 memory limit. Doesn't actually free anything (for now)
Download in the [releases tab](https://github.com/sims3fiend/S3MemoryFreer/releases)

# How to
I'm not sure if this will work on other versions of the game, as I only have access to the steam one. As the memory editing part is tied to a static address, I wouldn't be surprised if it messed something up on the origin/EA version, so give it a go first before needing to rely on it. It should be fine tho. The other feature(s) are fine to use.

You'll either need the [smooth patch ASI loader](https://modthesims.info/d/658759/smooth-patch.html) or [dxwrapper](https://github.com/elishacloud/dxwrapper)'s, then just pop the file into your `The Sims 3\Game\Bin` folder along with either of them. 
Don't use fullscreen mode otherwise you wont see the notification. If using dxwrapper, make sure you set LoadPlugins to 1

# Config / Features
The config file should be pretty self explanatory, but I'll go into more depth here about a couple of the values.

**Threshold** - This is the value that, if the game hits, will trigger the notification box to appear. I can't for the life of me figure out how to get an accurate read, so you need to assume there's about a 600~mb uncounted bonus on top of the value you set. Feel free to PR with a better solution 😭
**Also keep in mind that saving adds a lot of memory usage**. from testing this ranges from 200-800 mb, depending on where you save (map screen I hope)

**ModifyMemoryHotkey** - This is the hotkey for the memory modification feature. What this does is effectively memory edits the MaxActiveLots value thing to 0/not exist, which causes "High Detail Lots" to be unloaded This *can* free up some memory if the home lot is stuck loaded, but it's going to be a gain of like 10-100mb max.

**WorkingSetHotkey** - You shouldn't need to use this, but if you're getting E12'd and are desperate, you can press this to empty the working set, which will stop E12 from triggering. This will however not stop your game from crashing should you hit the games memory limit.

The other config values I need to delete, they were holdovers from when I thought clearing the working set actually freed memory.
# Reporting bugs
Please either create a new issue or message me on discord (@fleshtexture). Likewise, if you know of anything that clears up memory, or have any ideas, please get in touch.

Refer to my tumblr if you want to see some general tips, or the list of things I've tried/am working on. Here's a recent post. xoxo
