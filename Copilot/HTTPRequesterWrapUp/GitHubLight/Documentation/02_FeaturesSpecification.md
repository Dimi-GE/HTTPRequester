# GitHubLight Features Specification

## Core Features (MVP)

### 1. Branch Download
- Download specific branch from GitHub repository
- Support for public and private repositories (with token)
- Selective file filtering based on extensions
- Basic error handling for network issues

### 2. Content Extraction
- Extract downloaded content to temporary directory
- Preserve folder structure
- Handle file conflicts gracefully
- Memory-efficient extraction for large archives

### 3. Manifest Management
- Create local manifest (file list with checksums)
- Generate remote manifest from downloaded content
- Store manifests in JSON format for readability
- Version tracking for manifest changes

### 4. Difference Detection
- Compare local vs remote manifests
- Identify new, modified, and deleted files
- Generate difference report
- Export differences to readable format

### 5. File Updates
- Replace existing files with newer versions
- Add new files to project structure
- Optional backup of replaced files
- Validation of successful updates

### 6. Archive Creation
- Pack updated files into ZIP format
- Maintain folder structure in archive
- Compression optimization for upload
- Include manifest in archive

### 7. GitHub Upload
- Upload modified files back to repository
- Commit with automatic message generation
- Support for different branches
- Basic conflict resolution

## Enhanced Features (Post-MVP)

### Performance Features
- **Streaming Download/Upload**: Large file handling without memory overflow
- **Progress Bar Windows**: Visual feedback for long operations
- **Bandwidth Management**: Throttle network usage to prevent connection slowdown
- **Memory Management**: Efficient memory usage for large file operations
- **Incremental Sync**: Only process changed files

### User Experience Features
- **Multiple UE Version Support**: Compatibility across UE versions
- **Configuration Presets**: Save common repository settings
- **Operation History**: Track previous sync operations
- **Error Recovery**: Automatic retry mechanisms
- **Offline Mode**: Work with cached data when offline

### Developer Features
- **Custom File Filters**: Advanced filtering options
- **Merge Conflict Resolution**: Simple conflict resolution UI
- **Branch Management**: Switch between branches easily
- **Commit Message Templates**: Pre-defined commit message formats
- **Sync Scheduling**: Automated sync at intervals

### Quality Features
- **File Integrity Checks**: Verify file integrity after operations
- **Rollback Functionality**: Undo recent changes
- **Detailed Logging**: Comprehensive operation logs
- **Performance Metrics**: Track sync performance
- **Connection Testing**: Verify GitHub connectivity

## Technical Specifications

### Supported File Types
- Unreal Engine assets (.uasset, .umap, etc.)
- Source code files (.cpp, .h, .cs)
- Configuration files (.ini, .json, .xml)
- Documentation files (.md, .txt)
- Small media files (< 50MB each)

### Limitations
- **File Size Limit**: Individual files limited to 100MB
- **Repository Size**: Recommended max 1GB total
- **Network Dependency**: Requires stable internet connection
- **GitHub API Limits**: Respects GitHub rate limiting
- **UE Integration**: Editor-only functionality (no runtime)

### Dependencies
- HTTP/HTTPS library for API calls
- ZIP compression library (minizip-ng)
- JSON parsing library
- Unreal Engine Plugin API
- GitHub REST API v4

## Future Considerations
- Integration with other version control systems
- Team collaboration features
- Advanced merge capabilities
- Cloud storage alternatives
- Mobile platform support
