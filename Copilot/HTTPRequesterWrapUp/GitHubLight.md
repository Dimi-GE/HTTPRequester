I was getting through the project documentation to analyze its complexity. It seems like we are on different places of the same coin - while my intention was creating 2 plug-ins(relatively simple EasyZIp, and a bit more complex EasyGitHub), you evolved both projects to commercial level.

Let me clarify the overall idea:
1. Unreal Engine marketplace (Fab). While user's demand is unknown, it is the easiest place to start with after finalizing MVP. As both plug-ins are designed to be used exclusivelly within the Unreal Engine ecosystem, the Epic Games handles all the licenses, excepting those for external tools. Minizip-ng license at the same time allows using the library even in commercial projects. This would need to be double-checked, but we can assume it is true for now.

2. Simplicity over complexity. The EasyGitHub plug-in is designed with the idea of easy-to-use, or pkug-and-play in other words. It should serve as a reminder to sync your project files rather than full Git replacement. Same for the EasyZip. All the considerations, restrictions, and limitations should be listed.

3. Publishing. The GitHub is optional. I can use Pateron, Gumroad,, or any other platform for both plug-ins, or giveaway EasyZip, as it is not that much time-consuming. Excluding GitHub from publishing, there is no need in wrappers, or legal checks. Those plug-ins are not going worldwide as a professionally created software.

4. Team scale. I am a solo dev, without budget for investments. I have only my time so far. The only thing I can afford is 20£ per month to keep my license for agents alive.

5. Updated workflow. Don't update any documents, created so far. We will be back to those from time to time as those are a great starting point. However, we would need a few more. Let's split it to the following:

EasyGitHub. Create a separate folder for the project within the HTTPRequesterWrapUp. For this project, create a step-by-step plan considering the following specifications:
1) Easynes over complexity. The plug-in should handle the following:
- download the branch;
- extract content;
- create local and remote manifests;
- compare manifests in order to find differences;
- save differences to a separate file;
- update files (replacement of existing or adding if new);
- pack the updated files into ZIP;
- upload the files to the GitHub;
Generally speaking, we have all of those functions ready to be implemented. And that is it so far. i can rename the `EasyGitHub` to `GitHubLight`, stating that the plug-in is for light files only.

2) Features. I want you to list possible features, like the following:
- streaming downlad and upload;
- progress bar windows;
- bandwith management in order to not slow down the connection;
- memory management in order to not overload the system;
- support other UE versions;
Those features are complementary - they enhance and improve, but do not add extra functionality to the plug-in itself. And those kind of features I would like to see.

Conclusion:
We can leave the `ZipLight`(new naming, stating light weight and simplicity), and focus on `GitHubLight` for now - it will allow me to think through the plug-in once again and check all the licensing concerns.

In the `GitHubLight`folder, create the estimation document, similar to `02_NuancesDependenciesEffortEstimation.md` - estimate efforts, steps required, etc. Considering it for a solo dev with AI agents like yourself - it is the only thing I can afford.

What I won't to see specifically:
Likelyhood of achievement with criterias to boost progress;
Completing estimation with time investment 2h on a daily basis, and 4h in weekends;
A steb-by-step implementation plan, divided by weeks;
Folder per milestone structure, with weekly-based plan document for each.

Also, keep tracking the documentation for the chat separately - i want to have all the conversations saved in order to be able to rebuild the process when something will go wrong.