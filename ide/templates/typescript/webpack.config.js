const path = require('path');
const TerserPlugin = require('terser-webpack-plugin');
const CompressionPlugin = require('compression-webpack-plugin');
const { CleanWebpackPlugin } = require('clean-webpack-plugin');

module.exports = (env, argv) => {
  const isProduction = argv.mode === 'production';
  const isDevelopment = argv.mode === 'development';

  return {
    mode: argv.mode || 'development',
    entry: {
      main: './src/main.ts',
      backend: './src/backend.ts'
    },
    output: {
      path: path.resolve(__dirname, 'dist'),
      filename: isProduction ? '[name].[contenthash].js' : '[name].js',
      chunkFilename: isProduction ? '[name].[contenthash].chunk.js' : '[name].chunk.js',
      publicPath: '/',
      library: {
        type: 'umd',
        name: 'FoundryGame'
      }
    },
    resolve: {
      extensions: ['.ts', '.js', '.json'],
      alias: {
        '@': path.resolve(__dirname, 'src'),
        '@foundry/engine': path.resolve(__dirname, 'node_modules/@foundry/engine')
      }
    },
    module: {
      rules: [
        {
          test: /\.ts$/,
          use: [
            {
              loader: 'ts-loader',
              options: {
                configFile: path.resolve(__dirname, 'tsconfig.json'),
                transpileOnly: isDevelopment
              }
            }
          ],
          exclude: /node_modules/
        },
        {
          test: /\.(png|jpg|jpeg|gif|svg|webp)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/images/[name].[contenthash][ext]'
          }
        },
        {
          test: /\.(mp3|wav|ogg|m4a)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/sounds/[name].[contenthash][ext]'
          }
        },
        {
          test: /\.(gltf|glb|obj|fbx|dae)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/models/[name].[contenthash][ext]'
          }
        },
        {
          test: /\.(vert|frag|glsl)$/i,
          type: 'asset/source',
          generator: {
            filename: 'assets/shaders/[name].[contenthash][ext]'
          }
        },
        {
          test: /\.(json|xml|yaml|yml)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/data/[name].[contenthash][ext]'
          }
        },
        {
          test: /\.css$/i,
          use: ['style-loader', 'css-loader']
        },
        {
          test: /\.wasm$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/wasm/[name].[contenthash][ext]'
          }
        }
      ]
    },
    plugins: [
      new CleanWebpackPlugin(),
      ...(isProduction ? [
        new CompressionPlugin({
          algorithm: 'gzip',
          test: /\.(js|css|html|svg)$/,
          threshold: 10240,
          minRatio: 0.8
        }),
        new CompressionPlugin({
          algorithm: 'brotliCompress',
          test: /\.(js|css|html|svg)$/,
          threshold: 10240,
          minRatio: 0.8
        })
      ] : [])
    ],
    optimization: {
      minimize: isProduction,
      minimizer: [
        new TerserPlugin({
          terserOptions: {
            compress: {
              drop_console: isProduction,
              drop_debugger: isProduction,
              pure_funcs: isProduction ? ['console.log', 'console.info'] : []
            },
            mangle: {
              safari10: true
            }
          },
          extractComments: false
        })
      ],
      splitChunks: {
        chunks: 'all',
        cacheGroups: {
          vendor: {
            test: /[\\/]node_modules[\\/]/,
            name: 'vendors',
            chunks: 'all',
            priority: 10
          },
          engine: {
            test: /[\\/]@foundry[\\/]engine[\\/]/,
            name: 'foundry-engine',
            chunks: 'all',
            priority: 20
          }
        }
      },
      runtimeChunk: 'single'
    },
    devtool: isDevelopment ? 'eval-cheap-module-source-map' : 'source-map',
    devServer: {
      contentBase: path.join(__dirname, 'dist'),
      compress: true,
      port: 3000,
      host: 'localhost',
      open: true,
      hot: true,
      liveReload: true,
      client: {
        logging: 'info',
        overlay: {
          errors: true,
          warnings: false
        }
      },
      static: {
        directory: path.join(__dirname, 'assets'),
        publicPath: '/assets'
      },
      proxy: {
        '/api': {
          target: 'http://localhost:8080',
          changeOrigin: true,
          secure: false
        },
        '/ws': {
          target: 'ws://localhost:8080',
          changeOrigin: true,
          ws: true
        }
      }
    },
    performance: {
      hints: isProduction ? 'warning' : false,
      maxEntrypointSize: 512000,
      maxAssetSize: 512000
    },
    stats: {
      colors: true,
      modules: false,
      children: false,
      chunks: false,
      chunkModules: false
    }
  };
};