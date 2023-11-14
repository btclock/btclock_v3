import esbuild from "esbuild";
import { sassPlugin } from "esbuild-sass-plugin";
import htmlPlugin from '@chialab/esbuild-plugin-html';
import handlebarsPlugin from "esbuild-plugin-handlebars";
import { clean } from 'esbuild-plugin-clean';
import { copy } from 'esbuild-plugin-copy';

import postcss from "postcss";
import autoprefixer from "autoprefixer";

const hbsOptions = {
    additionalHelpers: { splitText: "helpers.js" },
    additionalPartials: {},
    precompileOptions: {}
}

esbuild
    .build({
        entryPoints: ["src/css/style.scss", "src/js/script.ts", "src/index.html", "src/js/helpers.js"],
        outdir: "build",
        bundle: true,
        loader: {
            ".png": "dataurl",
            ".woff": "dataurl",
            ".woff2": "dataurl",
            ".eot": "dataurl",
            ".ttf": "dataurl",
            ".svg": "dataurl",
        },
        plugins: [
            clean({
                patterns: ['./build/*']
            }),
            htmlPlugin(),
            sassPlugin({
                async transform(source) {
                    const { css } = await postcss([autoprefixer]).process(
                        source
                        , { from: undefined });
                    return css;
                },
            }),
            handlebarsPlugin(hbsOptions),
            copy({
                // this is equal to process.cwd(), which means we use cwd path as base path to resolve `to` path
                // if not specified, this plugin uses ESBuild.build outdir/outfile options as base path.
                resolveFrom: 'cwd',
                assets: {
                    from: ['./src/api.*'],
                    to: ['./build'],
                },
            })
        ],
        minify: true,
        metafile: false,
        sourcemap: false
    })
    .then(() => console.log("⚡ Build complete! ⚡"))
    .catch(() => process.exit(1));