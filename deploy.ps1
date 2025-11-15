# ============================================
# 直播流播放器自动打包脚本
# ============================================
# 此脚本会自动收集所有依赖并创建可分发的程序包

param(
    [string]$BuildType = "Release",
    [string]$Version = "1.0.0"
)

# ============================================
# 配置路径
# ============================================

# 项目根目录
$ProjectRoot = $PSScriptRoot

# FFmpeg 库路径（从 CMakeLists.txt 中读取）
$FFmpegRoot = "F:\Software\cpp_packages\ffmpeg-gpl"

# Qt 工具路径
$WinDeployQt = "D:\Software\qt5_12_9\5.12.9\msvc2017_64\bin\windeployqt.exe"

# 构建输出路径
$BuildDir = Join-Path $ProjectRoot "build"
$ReleaseDir = Join-Path $BuildDir $BuildType

# 部署目标路径
$DeployRoot = Join-Path $ProjectRoot "deploy"
$DeployDir = Join-Path $DeployRoot "LiveStreamPlayer_v${Version}"

# 可执行文件名
$ExeName = "09_LiveStreamPullPlayer.exe"

# ============================================
# 颜色输出函数
# ============================================
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

# ============================================
# 检查必需文件和路径
# ============================================
function Test-Prerequisites {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "正在检查打包前置条件..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    $allOk = $true
    
    # 检查可执行文件
    $exePath = Join-Path $ReleaseDir $ExeName
    if (-not (Test-Path $exePath)) {
        Write-ColorOutput "❌ 错误: 找不到可执行文件: $exePath" "Red"
        Write-ColorOutput "   请先使用 CMake 构建 $BuildType 版本" "Yellow"
        $allOk = $false
    }
    else {
        Write-ColorOutput "✓ 找到可执行文件: $exePath" "Green"
    }
    
    # 检查 FFmpeg
    if (-not (Test-Path $FFmpegRoot)) {
        Write-ColorOutput "❌ 错误: 找不到 FFmpeg 库: $FFmpegRoot" "Red"
        $allOk = $false
    }
    else {
        Write-ColorOutput "✓ 找到 FFmpeg 库: $FFmpegRoot" "Green"
    }
    
    # 检查 windeployqt
    if (-not (Test-Path $WinDeployQt)) {
        Write-ColorOutput "❌ 错误: 找不到 windeployqt 工具: $WinDeployQt" "Red"
        Write-ColorOutput "   请检查 Qt 安装路径" "Yellow"
        $allOk = $false
    }
    else {
        Write-ColorOutput "✓ 找到 windeployqt 工具: $WinDeployQt" "Green"
    }
    
    if (-not $allOk) {
        Write-ColorOutput "`n打包前置条件检查失败，请解决上述问题后重试。" "Red"
        exit 1
    }
    
    Write-ColorOutput "`n所有前置条件检查通过！`n" "Green"
}

# ============================================
# 清理并创建部署目录
# ============================================
function Initialize-DeployDirectory {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "准备部署目录..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    # 如果部署目录已存在，删除它
    if (Test-Path $DeployDir) {
        Write-ColorOutput "清理旧的部署目录..." "Yellow"
        Remove-Item -Path $DeployDir -Recurse -Force
    }
    
    # 创建新的部署目录
    New-Item -ItemType Directory -Path $DeployDir -Force | Out-Null
    Write-ColorOutput "✓ 创建部署目录: $DeployDir" "Green"
}

# ============================================
# 复制可执行文件
# ============================================
function Copy-Executable {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "复制可执行文件..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    $sourcePath = Join-Path $ReleaseDir $ExeName
    $destPath = Join-Path $DeployDir $ExeName
    
    Copy-Item -Path $sourcePath -Destination $destPath -Force
    Write-ColorOutput "✓ 已复制: $ExeName" "Green"
    
    # 显示文件大小
    $fileSize = (Get-Item $destPath).Length / 1MB
    Write-ColorOutput "  文件大小: $([math]::Round($fileSize, 2)) MB" "Gray"
}

# ============================================
# 部署 Qt 依赖
# ============================================
function Deploy-QtDependencies {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "部署 Qt 依赖库..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    $exePath = Join-Path $DeployDir $ExeName
    
    # 运行 windeployqt
    Write-ColorOutput "正在运行 windeployqt..." "Yellow"
    & $WinDeployQt $exePath --no-translations --no-system-d3d-compiler --no-opengl-sw
    
    if ($LASTEXITCODE -eq 0) {
        Write-ColorOutput "✓ Qt 依赖部署完成" "Green"
    }
    else {
        Write-ColorOutput "❌ Qt 依赖部署失败" "Red"
        exit 1
    }
}

# ============================================
# 复制 FFmpeg 依赖
# ============================================
function Copy-FFmpegDependencies {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "复制 FFmpeg 依赖库..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    $ffmpegBinDir = Join-Path $FFmpegRoot "bin"
    
    # 需要的 FFmpeg DLL 列表
    $requiredDlls = @(
        "avcodec-61.dll",
        "avformat-61.dll",
        "avutil-59.dll",
        "swscale-8.dll",
        "swresample-5.dll",
        "avfilter-10.dll",
        "avdevice-61.dll",
        "postproc-58.dll"
    )
    
    foreach ($dll in $requiredDlls) {
        $sourcePath = Join-Path $ffmpegBinDir $dll
        
        if (Test-Path $sourcePath) {
            $destPath = Join-Path $DeployDir $dll
            Copy-Item -Path $sourcePath -Destination $destPath -Force
            
            $fileSize = (Get-Item $destPath).Length / 1MB
            Write-ColorOutput "✓ 已复制: $dll ($([math]::Round($fileSize, 2)) MB)" "Green"
        }
        else {
            Write-ColorOutput "⚠ 警告: 找不到 $dll，跳过" "Yellow"
        }
    }
}

# ============================================
# 创建说明文档
# ============================================
function Create-Documentation {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "创建说明文档..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    # 创建 README.txt
    $readmePath = Join-Path $DeployDir "README.txt"
    $readmeContent = @"
====================================
直播流播放器 v${Version}
====================================

【使用说明】
1. 双击 09_LiveStreamPullPlayer.exe 启动播放器
2. 在 URL 输入框中输入直播流地址（支持 RTMP、HTTP、RTSP 等协议）
3. 点击"播放"按钮开始播放
4. 支持实时统计信息显示（帧率、码率等）

【支持的流媒体协议】
- RTMP (rtmp://)
- HTTP/HTTPS (http://, https://)
- RTSP (rtsp://)
- 本地文件 (file://)

【系统要求】
- Windows 10 或更高版本
- 64 位操作系统
- 建议 4GB 以上内存

【示例直播流地址】
可以使用以下公开测试流进行测试：
- rtmp://ns8.indexforce.com/home/mystream
- http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8

【常见问题】
Q: 播放器无法启动？
A: 请确保所有 DLL 文件都在同一目录下，不要移动或删除任何文件。

Q: 无法播放某些流？
A: 请检查网络连接和流地址是否正确，部分流可能需要特定的解码器支持。

Q: 画面卡顿？
A: 可能是网络带宽不足或设备性能有限，尝试降低流的分辨率。

【技术支持】
如有问题或建议，请联系开发者。

【版权声明】
本软件基于 FFmpeg 和 Qt 框架开发。
FFmpeg 采用 GPL 许可证。
"@
    
    Set-Content -Path $readmePath -Value $readmeContent -Encoding UTF8
    Write-ColorOutput "✓ 已创建: README.txt" "Green"
    
    # 复制 LICENSE
    $ffmpegLicense = Join-Path $FFmpegRoot "LICENSE.txt"
    if (Test-Path $ffmpegLicense) {
        Copy-Item -Path $ffmpegLicense -Destination (Join-Path $DeployDir "FFmpeg_LICENSE.txt") -Force
        Write-ColorOutput "✓ 已复制: FFmpeg_LICENSE.txt" "Green"
    }
}

# ============================================
# 统计部署包大小
# ============================================
function Show-DeploymentStats {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "部署统计信息" "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    # 统计文件数量
    $fileCount = (Get-ChildItem -Path $DeployDir -Recurse -File).Count
    Write-ColorOutput "文件总数: $fileCount" "White"
    
    # 统计总大小
    $totalSize = (Get-ChildItem -Path $DeployDir -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB
    Write-ColorOutput "总大小: $([math]::Round($totalSize, 2)) MB" "White"
    
    # 列出主要文件
    Write-ColorOutput "`n主要文件:" "White"
    Get-ChildItem -Path $DeployDir -File | 
    Sort-Object Length -Descending | 
    Select-Object -First 10 | 
    ForEach-Object {
        $size = $_.Length / 1MB
        Write-ColorOutput ("  {0,-30} {1,8:N2} MB" -f $_.Name, $size) "Gray"
    }
}

# ============================================
# 创建压缩包（可选）
# ============================================
function Create-ZipPackage {
    Write-ColorOutput "`n========================================" "Cyan"
    Write-ColorOutput "创建压缩包..." "Cyan"
    Write-ColorOutput "========================================`n" "Cyan"
    
    $zipPath = Join-Path $DeployRoot "LiveStreamPlayer_v${Version}.zip"
    
    # 如果 ZIP 已存在，删除它
    if (Test-Path $zipPath) {
        Remove-Item -Path $zipPath -Force
    }
    
    # 创建 ZIP 文件
    Write-ColorOutput "正在压缩文件，请稍候..." "Yellow"
    Compress-Archive -Path $DeployDir -DestinationPath $zipPath -CompressionLevel Optimal
    
    $zipSize = (Get-Item $zipPath).Length / 1MB
    Write-ColorOutput "✓ 已创建压缩包: $zipPath" "Green"
    Write-ColorOutput "  压缩包大小: $([math]::Round($zipSize, 2)) MB" "Gray"
}

# ============================================
# 主执行流程
# ============================================
function Main {
    $startTime = Get-Date
    
    Write-ColorOutput @"

╔══════════════════════════════════════════╗
║   直播流播放器自动打包脚本 v1.0          ║
╚══════════════════════════════════════════╝

"@ "Cyan"

    Write-ColorOutput "打包配置:" "White"
    Write-ColorOutput "  构建类型: $BuildType" "Gray"
    Write-ColorOutput "  版本号: $Version" "Gray"
    Write-ColorOutput "  目标目录: $DeployDir" "Gray"
    
    # 执行打包步骤
    Test-Prerequisites
    Initialize-DeployDirectory
    Copy-Executable
    Deploy-QtDependencies
    Copy-FFmpegDependencies
    Create-Documentation
    Show-DeploymentStats
    Create-ZipPackage
    
    # 计算耗时
    $elapsed = (Get-Date) - $startTime
    
    Write-ColorOutput "`n========================================" "Green"
    Write-ColorOutput "打包完成！" "Green"
    Write-ColorOutput "========================================`n" "Green"
    
    Write-ColorOutput "部署目录: $DeployDir" "White"
    Write-ColorOutput "耗时: $([math]::Round($elapsed.TotalSeconds, 2)) 秒" "Gray"
    
    Write-ColorOutput "`n您现在可以:" "Yellow"
    Write-ColorOutput "  1. 在 $DeployDir 中测试程序" "White"
    Write-ColorOutput "  2. 将整个文件夹分发给用户" "White"
    Write-ColorOutput "  3. 使用生成的 ZIP 压缩包进行分发" "White"
    
    # 询问是否打开部署目录
    Write-Host "`n是否打开部署目录？(Y/N): " -NoNewline -ForegroundColor Yellow
    $response = Read-Host
    if ($response -eq 'Y' -or $response -eq 'y') {
        Start-Process explorer.exe $DeployDir
    }
}

# 执行主函数
Main
