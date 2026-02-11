# Fuego Network Community Setup

This document provides instructions for setting up the community forum using Giscus.

## Prerequisites

1. GitHub Discussions must be enabled for the repository (already done ✅)
2. A GitHub account for users to participate in discussions

## Setting up Giscus

1. Visit [https://giscus.app](https://giscus.app)
2. Enter the repository name: `usexfg/usexfg.org`
3. Select the discussion category (typically "General")
4. Configure the following settings:
   - Mapping: "Use specific term" → "pathname"
   - reactions: Enable
   - Emit discussion metadata: Disable
   - Place the comment box: "Above the comments"
   - Theme: "Light"
   - Language: "English"

5. Copy the generated script configuration

## Updating the Configuration

After setting up Giscus, you may need to update the configuration in `community.html`:

1. Open `community.html`
2. Find the Giscus script section (around line 250)
3. Update the data attributes with the values provided by Giscus:
   - `data-repo`
   - `data-repo-id`
   - `data-category`
   - `data-category-id`

## Testing

1. Commit and push the changes to GitHub
2. Visit https://usexfg.org/community.html
3. Try to sign in with GitHub and leave a test comment

## Troubleshooting

If the comments section doesn't appear:

1. Ensure GitHub Discussions is enabled for the repository
2. Check that the data attributes in the script match the Giscus configuration
3. Verify that the repository is public (required for Giscus)
4. Check the browser console for any JavaScript errors

## Customization

You can customize the appearance by modifying:
- The color scheme in the CSS variables
- The layout and styling in the CSS section
- The content and structure in the HTML sections

## Privacy Considerations

Giscus is privacy-focused:
- No tracking or advertisements
- Comments are stored in GitHub Discussions
- Users must authenticate with GitHub to comment
- All data remains within the GitHub ecosystem