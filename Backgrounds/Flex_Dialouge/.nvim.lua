vim.api.nvim_set_keymap('n', '<leader>mb', ':!make<CR>', { noremap = true, silent = true })
vim.api.nvim_set_keymap('n', '<leader>mr', ':!/home/talgat/melonDS.AppImage "$(basename "$PWD").nds"<CR>', { noremap = true, silent = true })
vim.api.nvim_set_keymap('n', '<leader>mc', ':!make clean<CR>', { noremap = true, silent = true })

