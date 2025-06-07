import { jest } from '@jest/globals';

const mockParser = {
  setLanguage: jest.fn(),
  parse: jest.fn().mockReturnValue({
    rootNode: {
      hasError: jest.fn().mockReturnValue(false),
      descendantsOfType: jest.fn().mockReturnValue([]),
      children: [],
      startPosition: { row: 0, column: 0 },
      text: ''
    }
  }),
  createQuery: jest.fn().mockReturnValue({
    matches: jest.fn().mockReturnValue([])
  })
};

export default jest.fn(() => mockParser);
